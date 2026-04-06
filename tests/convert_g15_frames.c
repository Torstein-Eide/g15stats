#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libg15render.h>

static int write_pbm(const char *path, g15canvas *canvas) {
    FILE *fp = fopen(path, "w");
    int x;
    int y;

    if (fp == NULL) {
        return -1;
    }

    fprintf(fp, "P1\n%d %d\n", G15_LCD_WIDTH, G15_LCD_HEIGHT);
    for (y = 0; y < G15_LCD_HEIGHT; y++) {
        for (x = 0; x < G15_LCD_WIDTH; x++) {
            fprintf(fp, "%d", g15r_getPixel(canvas, x, y) ? 1 : 0);
            if (x + 1 < G15_LCD_WIDTH) {
                fputc(' ', fp);
            }
        }
        fputc('\n', fp);
    }

    fclose(fp);
    return 0;
}

static int write_ascii(const char *path, g15canvas *canvas) {
    FILE *fp = fopen(path, "w");
    int x;
    int y;

    if (fp == NULL) {
        return -1;
    }

    for (y = 0; y < G15_LCD_HEIGHT; y++) {
        for (x = 0; x < G15_LCD_WIDTH; x++) {
            fputc(g15r_getPixel(canvas, x, y) ? '#' : '.', fp);
        }
        fputc('\n', fp);
    }

    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    const char *in_path;
    const char *out_dir;
    FILE *in;
    unsigned char frame[G15_BUFFER_LEN];
    int frame_idx = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <input.bin> <out-dir>\n", argv[0]);
        return 2;
    }

    in_path = argv[1];
    out_dir = argv[2];

    in = fopen(in_path, "rb");
    if (in == NULL) {
        fprintf(stderr, "open %s failed: %s\n", in_path, strerror(errno));
        return 1;
    }

    while (fread(frame, 1, G15_BUFFER_LEN, in) == G15_BUFFER_LEN) {
        g15canvas canvas;
        char pbm_path[1024];
        char txt_path[1024];

        memset(&canvas, 0, sizeof(canvas));
        memcpy(canvas.buffer, frame, G15_BUFFER_LEN);

        snprintf(pbm_path, sizeof(pbm_path), "%s/frame_%03d.pbm", out_dir, frame_idx);
        snprintf(txt_path, sizeof(txt_path), "%s/frame_%03d.txt", out_dir, frame_idx);

        if (write_pbm(pbm_path, &canvas) != 0) {
            fclose(in);
            return 1;
        }
        if (write_ascii(txt_path, &canvas) != 0) {
            fclose(in);
            return 1;
        }

        frame_idx++;
    }

    fclose(in);
    return (frame_idx > 0) ? 0 : 1;
}
