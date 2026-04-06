#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef G15_PIXELBUF
#define G15_PIXELBUF 0
#endif
#ifndef G15_TEXTBUF
#define G15_TEXTBUF 1
#endif
#ifndef G15_WBMPBUF
#define G15_WBMPBUF 2
#endif
#ifndef G15_G15RBUF
#define G15_G15RBUF 3
#endif

typedef int (*new_g15_screen_fn)(int);
typedef int (*g15_send_fn)(int, char *, unsigned int);
typedef unsigned long (*g15_send_cmd_fn)(int, unsigned char, unsigned char);

static new_g15_screen_fn real_new_g15_screen = NULL;
static g15_send_fn real_g15_send = NULL;
static g15_send_cmd_fn real_g15_send_cmd = NULL;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct screen_info {
    int in_use;
    int sock;
    int screentype;
};

#define MAX_SCREENS 128
static struct screen_info screens[MAX_SCREENS];

static void init_syms(void)
{
    if (!real_new_g15_screen) {
        real_new_g15_screen = (new_g15_screen_fn)dlsym(RTLD_NEXT, "new_g15_screen");
        if (!real_new_g15_screen) {
            fprintf(stderr, "g15mirror: dlsym(new_g15_screen) failed: %s\n", dlerror());
            abort();
        }
    }
    if (!real_g15_send) {
        real_g15_send = (g15_send_fn)dlsym(RTLD_NEXT, "g15_send");
        if (!real_g15_send) {
            fprintf(stderr, "g15mirror: dlsym(g15_send) failed: %s\n", dlerror());
            abort();
        }
    }
    if (!real_g15_send_cmd) {
        real_g15_send_cmd = (g15_send_cmd_fn)dlsym(RTLD_NEXT, "g15_send_cmd");
        if (!real_g15_send_cmd) {
            /* Ikke abort her; noen klienter bruker kanskje ikke denne. */
        }
    }
}

static const char *screen_type_name(int t)
{
    switch (t) {
        case G15_PIXELBUF: return "PIXELBUF";
        case G15_TEXTBUF:  return "TEXTBUF";
        case G15_WBMPBUF:  return "WBMPBUF";
        case G15_G15RBUF:  return "G15RBUF";
        default:           return "UNKNOWN";
    }
}

static void remember_screen(int sock, int screentype)
{
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_SCREENS; i++) {
        if (!screens[i].in_use) {
            screens[i].in_use = 1;
            screens[i].sock = sock;
            screens[i].screentype = screentype;
            pthread_mutex_unlock(&lock);
            return;
        }
    }
    pthread_mutex_unlock(&lock);
}

static int lookup_screen_type(int sock)
{
    int out = -1;
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_SCREENS; i++) {
        if (screens[i].in_use && screens[i].sock == sock) {
            out = screens[i].screentype;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    return out;
}

static void make_timestamp(char *buf, size_t n)
{
    struct timespec ts;
    struct tm tmv;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tmv);
    snprintf(buf, n, "%04d%02d%02d-%02d%02d%02d-%03ld",
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ts.tv_nsec / 1000000);
}

static void write_pbm_p4_from_packed(const char *path, const unsigned char *buf, size_t len)
{
    /* G15 LCD er 160x43 => ceil(160/8)=20 bytes per rad => 860 bytes totalt */
    const int width = 160;
    const int height = 43;
    const int row_bytes = (width + 7) / 8;
    const size_t expected = (size_t)row_bytes * height;

    FILE *fp = fopen(path, "wb");
    if (!fp) return;

    fprintf(fp, "P4\n%d %d\n", width, height);

    if (len >= expected) {
        fwrite(buf, 1, expected, fp);
    } else {
        fwrite(buf, 1, len, fp);
        static unsigned char zeros[1024];
        size_t remain = expected - len;
        while (remain) {
            size_t chunk = remain > sizeof(zeros) ? sizeof(zeros) : remain;
            fwrite(zeros, 1, chunk, fp);
            remain -= chunk;
        }
    }

    fclose(fp);
}

static void dump_frame(int sock, int screentype, const unsigned char *buf, unsigned int len)
{
    const char *dir = getenv("G15MIRROR_DIR");
    if (!dir || !*dir) dir = "/tmp/g15mirror";

    char ts[64];
    make_timestamp(ts, sizeof(ts));

    char pathbin[512];
    snprintf(pathbin, sizeof(pathbin), "%s/frame-%s-sock%d-%s-%u.bin",
             dir, ts, sock, screen_type_name(screentype), len);

    FILE *fp = fopen(pathbin, "wb");
    if (fp) {
        fwrite(buf, 1, len, fp);
        fclose(fp);
    }

    /* Lag en PBM-preview for packed formater. */
    if (screentype == G15_WBMPBUF || screentype == G15_G15RBUF) {
        char pathpbm[512];
        snprintf(pathpbm, sizeof(pathpbm), "%s/frame-%s-sock%d-%s-%u.pbm",
                 dir, ts, sock, screen_type_name(screentype), len);
        write_pbm_p4_from_packed(pathpbm, buf, len);
    }

    /* ASCII-preview til stderr */
    if (screentype == G15_WBMPBUF || screentype == G15_G15RBUF) {
        const int width = 160;
        const int height = 43;
        const int row_bytes = (width + 7) / 8;
        fprintf(stderr, "\n[g15mirror] frame sock=%d type=%s len=%u\n",
                sock, screen_type_name(screentype), len);

        int max_rows = 12; /* ikke spam hele terminalen */
        for (int y = 0; y < height && y < max_rows; y++) {
            for (int x = 0; x < width; x++) {
                int byte_index = y * row_bytes + (x / 8);
                int bit_index = x % 8;
                int on = 0;
                if ((size_t)byte_index < len) {
                    on = (buf[byte_index] >> bit_index) & 1;
                }
                fputc(on ? '#' : '.', stderr);
            }
            fputc('\n', stderr);
        }
    } else if (screentype == G15_PIXELBUF) {
        fprintf(stderr, "[g15mirror] frame sock=%d type=%s len=%u (raw 1 byte/pixel)\n",
                sock, screen_type_name(screentype), len);
    } else {
        fprintf(stderr, "[g15mirror] frame sock=%d type=%s len=%u\n",
                sock, screen_type_name(screentype), len);
    }
}

int new_g15_screen(int screentype)
{
    init_syms();
    int sock = real_new_g15_screen(screentype);
    if (sock >= 0) {
        remember_screen(sock, screentype);
        fprintf(stderr, "[g15mirror] new_g15_screen sock=%d type=%s(%d)\n",
                sock, screen_type_name(screentype), screentype);
    }
    return sock;
}

int g15_send(int sock, char *buf, unsigned int len)
{
    init_syms();

    int screentype = lookup_screen_type(sock);
    if (buf && len > 0) {
        dump_frame(sock, screentype, (const unsigned char *)buf, len);
    }

    return real_g15_send(sock, buf, len);
}

unsigned long g15_send_cmd(int sock, unsigned char command, unsigned char value)
{
    init_syms();

    fprintf(stderr, "[g15mirror] cmd sock=%d command=0x%02x value=0x%02x\n",
            sock, command, value);

    if (!real_g15_send_cmd) {
        errno = ENOSYS;
        return (unsigned long)-1;
    }

    return real_g15_send_cmd(sock, command, value);
}