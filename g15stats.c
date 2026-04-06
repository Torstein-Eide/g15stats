/*
This file is part of g15daemon.

g15daemon is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

g15daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with g15daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

(c) 2008-2010 Mike Lampard

$Revision: 538 $ -  $Date: 2011-04-27 23:23:15 +0200 (Mi, 27 Apr 2011) $ $Author: czarnyckm $

This daemon listens on localhost port 15550 for client connections,
and arbitrates LCD display.  Allows for multiple simultaneous clients.
Client screens can be cycled through by pressing the 'L1' key.

This is a simple stats client showing graphs for CPU, MEM & Swap usage, Network traffic, Temperatures, CPU Frequency and Battery life.
*/
#define _GNU_SOURCE 1

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <libg15.h>
#include <ctype.h>
#include <g15daemon_client.h>
#include <libg15render.h>
#include <sched.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/sysdeps.h> 
#include <glibtop/netload.h> 
#include <glibtop/swap.h> 
#include <glibtop/loadavg.h>
#include <glibtop/uptime.h>
#include <glibtop/sysinfo.h>
#include <yaml.h>
#include "g15stats.h"

int g15screen_fd;

int cycle       = 0;
int info_cycle  = 0;
/** Holds the mode type variable of the application running */
int mode[MAX_SCREENS + 1];
/** Holds the sub mode type variable of the application running */
int submode     = 1;

int info_cycle_timer    = -1;

// Summary Screen indicators count 4 or 5
int summary_rows    = 4;

int have_freq   = 2;

int wait_seconds = 1;

_Bool have_temp = 1;
_Bool have_fan  = 1;
_Bool have_bat  = 1;
_Bool have_nic  = 0;
_Bool variable_cpu = 0;
_Bool debug_enabled = 0;
_Bool show_screen_overlay = 1;
_Bool bar_chart_background = 0;

int overlay_screen = -1;
int overlay_ticks = 0;
int cpu2_min_bar_width = 3;
int cpu2_bar_height = 3;
int cpu2_debug_logged = 0;
int mode_overlay_ticks = 0;
char mode_overlay_text[96] = {0};

_Bool sensor_type_temp[MAX_SENSOR];
_Bool sensor_type_fan[MAX_SENSOR];

int sensor_lost_temp[MAX_SENSOR];
int sensor_lost_fan[MAX_SENSOR];
int sensor_lost_bat     = 1;

int sensor_temp_id      = 0;
int sensor_temp_main    = 0;

int sensor_fan_id       = 0;

_Bool sensor_temp_forced = 0;
_Bool sensor_fan_forced = 0;
_Bool cpu_vendor_amd = 0;
_Bool cpu_vendor_intel = 0;
_Bool debug_cpu_temp_logged = 0;

/** Holds the number of the cpus */
int ncpu;

unsigned int net_hist[MAX_NET_HIST][2];
int net_rr_index=0;

unsigned long net_max_in    = 100;
unsigned long net_max_out   = 100;

unsigned long net_cur_in    = 0;
unsigned long net_cur_out   = 0;

float temp_tot_cur  = 1;
float temp_tot_max  = 1;

float fan_tot_cur   = 1;
float fan_tot_max   = 1;

_Bool net_scale_absolute=0;

pthread_cond_t wake_now = PTHREAD_COND_INITIALIZER;

#define G15STATS_CONFIG_FILE "/etc/g15plugins/g15stats.yaml"

static const char *get_config_file_path(void) {
    const char *env_path = getenv("G15STATS_CONFIG_FILE");

    if (env_path != NULL && env_path[0] != '\0') {
        return env_path;
    }

    return G15STATS_CONFIG_FILE;
}

static int parse_bool_value(const char *value) {
    if (value == NULL) {
        return 0;
    }

    return (strcasecmp(value, "true") == 0 ||
            strcasecmp(value, "yes") == 0 ||
            strcmp(value, "1") == 0 ||
            strcasecmp(value, "on") == 0);
}

static int parse_int_value(const char *value, int *parsed) {
    char *endptr;
    long parsed_long;

    if (value == NULL || parsed == NULL) {
        return 0;
    }

    parsed_long = strtol(value, &endptr, 10);
    if (endptr == value || *endptr != '\0') {
        return 0;
    }

    *parsed = (int)parsed_long;
    return 1;
}

static int get_info_pause(void) {
    const char *env_pause = getenv("G15STATS_PAUSE");
    int parsed_pause = 0;

    if (parse_int_value(env_pause, &parsed_pause) && parsed_pause > 0) {
        return parsed_pause;
    }

    return PAUSE;
}

static int get_forced_screen(void) {
    const char *env_screen = getenv("G15STATS_FORCE_SCREEN");
    int parsed_screen = -1;

    if (parse_int_value(env_screen, &parsed_screen)
            && parsed_screen >= SCREEN_SUMMARY
            && parsed_screen <= MAX_SCREENS) {
        return parsed_screen;
    }

    return -1;
}

static int get_forced_mode(void) {
    const char *env_mode = getenv("G15STATS_FORCE_MODE");
    int parsed_mode = -1;

    if (parse_int_value(env_mode, &parsed_mode)
            && parsed_mode >= 0
            && parsed_mode <= MAX_MODE) {
        return parsed_mode;
    }

    return -1;
}

static const char *screen_name(int screen_id) {
    switch (screen_id) {
        case SCREEN_SUMMARY: return "SUMMARY";
        case SCREEN_CPU: return "CPU LOAD";
        case SCREEN_CPU2: return "CPU LOAD2";
        case SCREEN_FREQ_AGG: return "CPU FREQ AGG";
        case SCREEN_MEM: return "MEMORY";
        case SCREEN_SWAP: return "SWAP";
        case SCREEN_NET: return "NETWORK";
        case SCREEN_BAT: return "BATTERY";
        case SCREEN_TEMP: return "TEMPERATURE";
        case SCREEN_FAN: return "FAN";
        case SCREEN_NET2: return "NETWORK PEAK";
        default: return "UNKNOWN";
    }
}

static int screen_is_visible(int screen_id) {
    switch (screen_id) {
        case SCREEN_NET:
        case SCREEN_NET2:
            return have_nic;
        case SCREEN_BAT:
            return have_bat;
        case SCREEN_TEMP:
            return have_temp;
        case SCREEN_FAN:
            return have_fan;
        default:
            return 1;
    }
}

static int find_next_visible_screen(int current, int direction) {
    int next = current;
    int steps = 0;

    if (direction == 0) {
        return current;
    }

    do {
        next += direction;
        if (next > MAX_SCREENS) {
            next = SCREEN_SUMMARY;
        } else if (next < SCREEN_SUMMARY) {
            next = MAX_SCREENS;
        }
        if (screen_is_visible(next)) {
            return next;
        }
        steps++;
    } while (steps <= MAX_SCREENS);

    return current;
}

static int overlay_screen_number(int screen_id) {
    int s;
    int number = 0;

    for (s = SCREEN_SUMMARY; s <= screen_id; s++) {
        if (screen_is_visible(s)) {
            number++;
        }
    }

    if (number <= 0) {
        number = screen_id + 1;
    }

    return number;
}

static const char *mode_description(int screen_id, int mode_value) {
    switch (screen_id) {
        case SCREEN_NET:
            return mode_value ? "NET: absolute scale" : "NET: autoscale";
        case SCREEN_CPU2:
            return mode_value ? "CPU2: vertical bars" : "CPU2: horizontal bars";
        case SCREEN_CPU:
            return mode_value ? "CPU: detailed bars" : "CPU: simple bars";
        default:
            return mode_value ? "Mode: 1" : "Mode: 0";
    }
}

static const char *submode_description(int submode_value) {
    return submode_value ? "Submode: fixed info" : "Submode: rotate info";
}

static void queue_mode_overlay(const char *text) {
    if (text == NULL || text[0] == '\0') {
        return;
    }

    snprintf(mode_overlay_text, sizeof(mode_overlay_text), "%s", text);
    mode_overlay_ticks = 2;
}

static void apply_config_value(const char *key,
                               const char *value,
                               int *go_daemon,
                               int *unicore,
                               unsigned char *interface,
                               _Bool *nic_enabled,
                               char *output_file_path,
                               size_t output_file_path_len) {
    int parsed;

    if (strcmp(key, "daemon") == 0) {
        *go_daemon = parse_bool_value(value);
    } else if (strcmp(key, "unicore") == 0) {
        *unicore = parse_bool_value(value);
    } else if (strcmp(key, "net_scale_absolute") == 0) {
        net_scale_absolute = parse_bool_value(value);
    } else if (strcmp(key, "disable_freq") == 0) {
        have_freq = parse_bool_value(value) ? 0 : 2;
    } else if (strcmp(key, "info_rotate") == 0) {
        if (parse_bool_value(value)) {
            submode = 0;
        }
    } else if (strcmp(key, "variable_cpu") == 0) {
        variable_cpu = parse_bool_value(value);
    } else if (strcmp(key, "debug") == 0) {
        debug_enabled = parse_bool_value(value);
    } else if (strcmp(key, "screen_overlay") == 0) {
        show_screen_overlay = parse_bool_value(value);
    } else if (strcmp(key, "bar_background") == 0) {
        bar_chart_background = parse_bool_value(value);
    } else if (strcmp(key, "cpu2_min_bar_width") == 0) {
        if (parse_int_value(value, &parsed)
                && parsed >= 1
                && parsed <= (BAR_END - BAR_START + 1)) {
            cpu2_min_bar_width = parsed;
        }
    } else if (strcmp(key, "cpu2_bar_height") == 0) {
        if (parse_int_value(value, &parsed)
                && parsed >= 1
                && parsed <= (BAR_BOTTOM + 1)) {
            cpu2_bar_height = parsed;
        }
    } else if (strcmp(key, "interface") == 0) {
        if (strlen(value) > 0) {
            size_t iface_len = strlen(value);
            if (iface_len > 127) {
                iface_len = 127;
            }
            memcpy((char *)interface, value, iface_len);
            interface[iface_len] = '\0';
            *nic_enabled = 1;
        }
    } else if (strcmp(key, "output_file") == 0) {
        if (value[0] != '\0' && output_file_path != NULL && output_file_path_len > 0) {
            strncpy(output_file_path, value, output_file_path_len - 1);
            output_file_path[output_file_path_len - 1] = '\0';
        }
    } else if (strcmp(key, "refresh") == 0) {
        if (parse_int_value(value, &parsed) && parsed >= 1 && parsed <= MAX_INTERVAL) {
            wait_seconds = parsed;
        }
    } else if (strcmp(key, "temperature") == 0) {
        if (parse_int_value(value, &parsed) && parsed >= 0 && parsed < MAX_SENSOR) {
            sensor_temp_id = parsed;
            sensor_temp_forced = 1;
        }
    } else if (strcmp(key, "global_temp") == 0) {
        if (parse_int_value(value, &parsed) && parsed >= 0 && parsed < MAX_SENSOR) {
            sensor_temp_main = parsed;
        }
    } else if (strcmp(key, "fan") == 0) {
        if (parse_int_value(value, &parsed) && parsed >= 0 && parsed < MAX_SENSOR) {
            sensor_fan_id = parsed;
            sensor_fan_forced = 1;
        }
    }
}

static void load_config_file(int *go_daemon,
                             int *unicore,
                             unsigned char *interface,
                             _Bool *nic_enabled,
                             char *output_file_path,
                             size_t output_file_path_len) {
    yaml_parser_t parser;
    yaml_event_t event;
    FILE *fp;
    int done = 0;
    int parse_error = 0;
    int expecting_key = 1;
    int mapping_depth = 0;
    int sequence_depth = 0;
    char current_key[128] = {0};

    const char *config_file = get_config_file_path();

    fp = fopen(config_file, "r");
    if (fp == NULL) {
        return;
    }

    if (!yaml_parser_initialize(&parser)) {
        fclose(fp);
        return;
    }

    yaml_parser_set_input_file(&parser, fp);

    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            fprintf(stderr,
                    "Warning: invalid YAML in %s at line %lu, column %lu\n",
                    config_file,
                    (unsigned long) parser.problem_mark.line + 1,
                    (unsigned long) parser.problem_mark.column + 1);
            parse_error = 1;
            break;
        }

        switch (event.type) {
            case YAML_MAPPING_START_EVENT:
                if (mapping_depth == 1 && expecting_key == 0 && sequence_depth == 0) {
                    fprintf(stderr,
                            "Warning: unsupported nested map value for key '%s' in %s\n",
                            current_key,
                            config_file);
                    expecting_key = 1;
                }
                mapping_depth++;
                break;
            case YAML_SEQUENCE_START_EVENT:
                if (mapping_depth == 1 && expecting_key == 0) {
                    fprintf(stderr,
                            "Warning: unsupported sequence value for key '%s' in %s\n",
                            current_key,
                            config_file);
                    expecting_key = 1;
                }
                sequence_depth++;
                break;
            case YAML_SEQUENCE_END_EVENT:
                if (sequence_depth > 0) {
                    sequence_depth--;
                }
                break;
            case YAML_MAPPING_END_EVENT:
                if (mapping_depth > 0) {
                    mapping_depth--;
                }
                break;
            case YAML_SCALAR_EVENT:
                if (mapping_depth == 1 && sequence_depth == 0) {
                    size_t value_len = event.data.scalar.length;
                    if (expecting_key) {
                        if (value_len >= sizeof(current_key)) {
                            value_len = sizeof(current_key) - 1;
                        }
                        memcpy(current_key, event.data.scalar.value, value_len);
                        current_key[value_len] = '\0';
                        expecting_key = 0;
                    } else {
                        char scalar_value[256];
                        if (value_len >= sizeof(scalar_value)) {
                            value_len = sizeof(scalar_value) - 1;
                        }
                        memcpy(scalar_value, event.data.scalar.value, value_len);
                        scalar_value[value_len] = '\0';
                        apply_config_value(current_key,
                                           scalar_value,
                                           go_daemon,
                                           unicore,
                                           interface,
                                           nic_enabled,
                                           output_file_path,
                                           output_file_path_len);
                        expecting_key = 1;
                    }
                }
                break;
            case YAML_STREAM_END_EVENT:
                done = 1;
                break;
            default:
                break;
        }

        yaml_event_delete(&event);
    }

    if (!done && !parse_error) {
        while (!done && yaml_parser_parse(&parser, &event)) {
            done = (event.type == YAML_STREAM_END_EVENT);
            yaml_event_delete(&event);
        }
    }

    yaml_parser_delete(&parser);
    fclose(fp);
}

unsigned long maxi(unsigned long a, unsigned long b) {
  if(a>b)
    return a;
  return b;
}

void format_float(char *tmpstr, const char *format_less, const char *format_great, float value){
    if (value < 9.9) {
        sprintf(tmpstr,format_less,value);
      } else {
        sprintf(tmpstr,format_great,(unsigned long)value);
      }
}

void append_text(char *dst, size_t dst_size, const char *src) {
    size_t len;

    if (dst == NULL || src == NULL || dst_size == 0) {
        return;
    }

    len = strlen(dst);
    if (len >= (dst_size - 1)) {
        return;
    }

    snprintf(dst + len, dst_size - len, "%s", src);
}

void append_textf(char *dst, size_t dst_size, const char *format, ...) {
    size_t len;
    va_list args;

    if (dst == NULL || format == NULL || dst_size == 0) {
        return;
    }

    len = strlen(dst);
    if (len >= (dst_size - 1)) {
        return;
    }

    va_start(args, format);
    vsnprintf(dst + len, dst_size - len, format, args);
    va_end(args);
}

char * show_bytes_short(unsigned long bytes) {
    static char tmpstr[32];
    if(bytes>=1024*1024) {
      format_float(tmpstr, "%4.1fM","%4luM", (float)bytes / (1024*1024));
    }
    else if(bytes >= 1024) {
      format_float(tmpstr, "%4.1fk","%4luk", (float)bytes / 1024);
    }
    else {
        sprintf(tmpstr,"%4luB",bytes);
    }
    return tmpstr;
}


char * show_bytes(unsigned long bytes) {
    static char tmpstr[32];
    if(bytes>=1024*1024) {
      format_float(tmpstr, "%2.1fMB","%luMB", (float)bytes / (1024*1024));
    }
    else if(bytes >= 1024) {
      format_float(tmpstr, "%2.1fkB","%lukB", (float)bytes / 1024);
    }
    else {
        sprintf(tmpstr,"%luB",bytes);
    }
    return tmpstr;
}

char * show_hertz_logic(int hertz, const char *hz) {
    static char tmpstr[32];
    if (hertz >= 1000000) {
        format_float(tmpstr, "%3.1fG","%3.fG", (float)hertz / 1000000);
    } else if (hertz >= 1000) {
        format_float(tmpstr, "%3.1fM","%3luM", (float)hertz / 1000);
    } else {
        sprintf(tmpstr, "%3iK", hertz);
    }
    if (hz[0] != '\0') {
        append_text(tmpstr, sizeof(tmpstr), hz);
    }
    return tmpstr;
}

char * show_hertz_short(int hertz) {
    return show_hertz_logic(hertz, "");
}

char * show_hertz(int hertz) {
    return show_hertz_logic(hertz, "Hz");
}

void print_vert_label_logic(g15canvas *canvas, const char *label, unsigned int sx){
    int i;
    int len = strlen(label);
    if (len > 5) {
        len = 5;
    }
    int starty = 0;
    int incy   = 0;
    switch (len){
        case    6   :
            break;
        case    5   :
            starty  = 0;
            break;
        case    4   :
            starty  = 4;
            break;
        case    3   :
            starty  = 3;
            incy    = 4;
            break;
        default :
            return;
    }
    char val[2];
    for (i=0; i<len; i++) {
        val[0] = label[i];
        val[1] = '\0';
        g15r_renderString (canvas, (unsigned char*)val, i, G15_TEXT_MED, sx, starty+(i*incy));
    }
}

void print_vert_label(g15canvas *canvas, const char *label){
    print_vert_label_logic(canvas, label, TEXT_RIGHT);
}

void init_battery_sensor(void) {
    int i;

    for (i = 0; i < NUM_BATS; i++) {
        FILE *fd_state;
        char filename[30];

        snprintf(filename, sizeof(filename), "/proc/acpi/battery/BAT%d/state", i);
        fd_state = fopen(filename, "r");
        if (fd_state != NULL) {
            fclose(fd_state);
            sensor_lost_bat = RETRY_COUNT;
            return;
        }
    }

    have_bat = 0;
    printf("Battery sensor doesn't appear to exist. Battery screen will be disabled.\n");
}

void drawBar_reversed (g15canvas * canvas, int x1, int y1, int x2, int y2, int color,
                       int num, int max, int type)
{
    float len, length;

    if (!bar_chart_background && type == 5) {
        return;
    }

    if (max <= 0 || num <= 0)
        return;
    if (num > max)
        num = max;

    if (type == 2)
    {
        y1 += 2;
        y2 -= 2;
        x1 += 2;
        x2 -= 2;
    }

    len = ((float) max / (float) num);
    length = (x2 - x1) / len;

    if (type == 1)
    {
        g15r_pixelBox (canvas, x1, y1 - type, x2, y2 + type, color ^ 1, 1, 1);
        g15r_pixelBox (canvas, x1, y1 - type, x2, y2 + type, color, 1, 0);
    }
    else if (type == 2)
    {
        g15r_pixelBox (canvas, x1 - 2, y1 - type, x2 + 2, y2 + type, color ^ 1,
                       1, 1);
        g15r_pixelBox (canvas, x1 - 2, y1 - type, x2 + 2, y2 + type, color, 1,
                       0);
    }
    else if (type == 3)
    {
        g15r_drawLine (canvas, x1, y1 - type, x1, y2 + type, color);
        g15r_drawLine (canvas, x2, y1 - type, x2, y2 + type, color);
        g15r_drawLine (canvas, x1, y1 + ((y2 - y1) / 2), x2,
                       y1 + ((y2 - y1) / 2), color);
    }
    g15r_pixelBox (canvas, (int) ceil (x2-length), y1, x2, y2, color, 1, 1);
    if(type == 5) {
        int x;
        for(x=x2-2;x>(x2-length);x-=2)
            g15r_drawLine (canvas, x, y1, x, y2, color^1);
    }
}

void drawBar_both(g15canvas *canvas, int y1, int y2, int bar_current, int bar_total, int rbar_current, int rbar_total) {
    g15r_drawBar(canvas, BAR_START, y1, BAR_END, y2, G15_COLOR_BLACK, bar_current, bar_total, 4);
    drawBar_reversed(canvas, BAR_START, y1, BAR_END, y2, G15_COLOR_BLACK, rbar_current, rbar_total, 5);
}

void drawLine_both(g15canvas *canvas, int y1, int y2) {
    g15r_drawLine(canvas, VL_LEFT, y1, VL_LEFT, y2, G15_COLOR_BLACK);
    g15r_drawLine(canvas, VL_LEFT + 1, y1, VL_LEFT + 1, y2, G15_COLOR_BLACK);
}

void drawAll_both(g15canvas *canvas, int y1, int y2, int bar_current, int bar_total, int rbar_current, int rbar_total) {
    drawBar_both(canvas, y1, y2, bar_current, bar_total, rbar_current, rbar_total);
    drawLine_both(canvas, y1, y2);
}

int daemonise(int nochdir, int noclose) {
    pid_t pid;

    if (nochdir < 1 && chdir("/") != 0) {
        return -1;
    }
    pid = fork();

    switch(pid){
        case -1:
            printf("Unable to daemonise!\n");
            return -1;
            break;
            case 0: {
                umask(0);
                if(setsid()==-1) {
                    perror("setsid");
                    return -1;
                }
                if(noclose<1) {
                    if (freopen("/dev/null", "r", stdin) == NULL
                            || freopen("/dev/null", "w", stdout) == NULL
                            || freopen("/dev/null", "w", stderr) == NULL) {
                        return -1;
                    }
                }
                break;
            }
        default:
            _exit(0);
    }
    return 0;
}

void init_cpu_count(void) {
    if ((variable_cpu) || (!ncpu)) {
        const glibtop_sysinfo *cpuinfo = glibtop_get_sysinfo();

        if(cpuinfo->ncpu == 0) {
            ncpu = 1;
        } else {
            ncpu = cpuinfo->ncpu;
        }
    }
}

int get_sysfs_value(const char *filename) {
    int ret_val = -1;
    FILE *fd_main;
    static char tmpstr [MAX_LINES];

    fd_main = fopen(filename, "r");
    if (fd_main != NULL) {
        if (fgets(tmpstr, MAX_LINES, fd_main) != NULL) {
            fclose(fd_main);
            ret_val = atoi(tmpstr);
        } else
            fclose(fd_main);
    } else {
        ret_val = SENSOR_ERROR;
    }
    return ret_val;
}

int get_sysfs_text(const char *filename, char *out, size_t out_size) {
    FILE *fd_main;

    if (out == NULL || out_size == 0) {
        return SENSOR_ERROR;
    }
    out[0] = '\0';

    fd_main = fopen(filename, "r");
    if (fd_main == NULL) {
        return SENSOR_ERROR;
    }

    if (fgets(out, (int) out_size, fd_main) == NULL) {
        fclose(fd_main);
        return SENSOR_ERROR;
    }
    fclose(fd_main);

    out[strcspn(out, "\r\n")] = '\0';
    return 0;
}

int get_processor_freq(const char *which, int core) {
    static char tmpstr [MAX_LINES];
    sprintf(tmpstr, "/sys/devices/system/cpu/cpu%d/cpufreq/%s", core, which);
    return get_sysfs_value(tmpstr);
}

int get_cpu_freq_cur(int core) {
    int ret_val;
    switch (have_freq) {
        case 2:
            ret_val = get_processor_freq("scaling_cur_freq", core);
            break;
        default:
            ret_val = get_processor_freq("cpuinfo_%d_freq", core);
            break;
    }
    if ((!core) && (ret_val == SENSOR_ERROR)) {
        have_freq--;
        if (have_freq < 0) {
            have_freq = 0;
        }
        printf("Frequency sensor doesn't appear to exist for core=%d . ", core);
    }
    return ret_val;
}

int get_cpu_freq_max(int core) {
    return get_processor_freq("cpuinfo_max_freq", core);
}

int get_cpu_freq_min(int core) {
    int ret_val;

    ret_val = get_processor_freq("scaling_min_freq", core);
    if (ret_val == SENSOR_ERROR) {
        ret_val = get_processor_freq("cpuinfo_min_freq", core);
    }

    return ret_val;
}

int get_hwmon(int sensor_id, const char *sensor, const char *which, int id, _Bool sensor_type) {
    char tmpstr [MAX_LINES];
    if(sensor_type) {
        sprintf(tmpstr, "/sys/class/hwmon/hwmon%d/device/%s%d_%s", sensor_id, sensor, id, which);
    } else {
        sprintf(tmpstr, "/sys/class/hwmon/hwmon%d/%s%d_%s", sensor_id, sensor, id, which);
    }
    return get_sysfs_value(tmpstr);
}

int get_hwmon_text(int sensor_id, const char *sensor, const char *which, int id, _Bool sensor_type, char *out, size_t out_size) {
    char tmpstr[MAX_LINES];

    if ((sensor == NULL || sensor[0] == '\0') && (id <= 0)) {
        if (sensor_type) {
            snprintf(tmpstr, sizeof(tmpstr), "/sys/class/hwmon/hwmon%d/device/%s", sensor_id, which);
        } else {
            snprintf(tmpstr, sizeof(tmpstr), "/sys/class/hwmon/hwmon%d/%s", sensor_id, which);
        }
    } else {
        if (sensor_type) {
            snprintf(tmpstr, sizeof(tmpstr), "/sys/class/hwmon/hwmon%d/device/%s%d_%s", sensor_id, sensor, id, which);
        } else {
            snprintf(tmpstr, sizeof(tmpstr), "/sys/class/hwmon/hwmon%d/%s%d_%s", sensor_id, sensor, id, which);
        }
    }

    return get_sysfs_text(tmpstr, out, out_size);
}

int is_amd_preferred_temp_label(const char *label) {
    if (label == NULL) {
        return 0;
    }

    return (strncasecmp(label, "tctl", 4) == 0 || strncasecmp(label, "tccd", 4) == 0);
}

void init_cpu_vendor_flags(void) {
    FILE *fd;
    char line[256];

    cpu_vendor_amd = 0;
    cpu_vendor_intel = 0;
    fd = fopen("/proc/cpuinfo", "r");
    if (fd == NULL) {
        return;
    }

    while (fgets(line, sizeof(line), fd) != NULL) {
        if (strstr(line, "AuthenticAMD") != NULL) {
            cpu_vendor_amd = 1;
        }
        if (strstr(line, "GenuineIntel") != NULL) {
            cpu_vendor_intel = 1;
        }
        if (cpu_vendor_amd || cpu_vendor_intel) {
            break;
        }
    }

    fclose(fd);

    if (debug_enabled) {
        if (cpu_vendor_amd) {
            fprintf(stderr,
                    "[g15stats] cpu vendor detected: AMD (temp auto prefers hwmon name 'k10temp'; summary auto prefers max(Tctl,Tccd*))\n");
        } else if (cpu_vendor_intel) {
            fprintf(stderr,
                    "[g15stats] cpu vendor detected: Intel (temp auto prefers hwmon name 'coretemp')\n");
        } else {
            fprintf(stderr,
                    "[g15stats] cpu vendor detected: unknown (temp auto uses generic best-probe selection)\n");
        }
    }
}

int hwmon_name_matches(int sensor_id, _Bool sensor_type, const char *needle) {
    char name[64];

    if (needle == NULL || needle[0] == '\0') {
        return 0;
    }

    if (get_hwmon_text(sensor_id, "", "name", 0, sensor_type, name, sizeof(name)) == 0) {
        if (strstr(name, needle) != NULL) {
            return 1;
        }
    }

    return 0;
}

int fan_label_is_preferred(const char *label) {
    if (label == NULL) {
        return 0;
    }

    return (strcasestr(label, "cpu") != NULL
            || strcasestr(label, "processor") != NULL
            || strcasestr(label, "soc") != NULL);
}

int count_preferred_fan_labels(int sensor_id, _Bool sensor_type) {
    int id;
    int score = 0;

    for (id = 1; id <= NUM_PROBES; id++) {
        char fan_label[64];

        if (get_hwmon_text(sensor_id,
                           "fan",
                           "label",
                           id,
                           sensor_type,
                           fan_label,
                           sizeof(fan_label)) != 0) {
            continue;
        }

        if (fan_label_is_preferred(fan_label)) {
            score++;
        }
    }

    return score;
}

int get_sensor_cur(int id, int screen_id) {
    if (screen_id == SCREEN_TEMP) {
        return get_hwmon(sensor_temp_id, "temp", "input", id, sensor_type_temp[sensor_temp_id]);
    } else {
        return get_hwmon(sensor_fan_id, "fan", "input", id, sensor_type_fan[sensor_fan_id]);
    }
}

int get_sensor_max(int id, int screen_id) {
    if (screen_id == SCREEN_TEMP) {
        return get_hwmon(sensor_temp_id, "temp", "max", id, sensor_type_temp[sensor_temp_id]);
    } else {
        return get_hwmon(sensor_fan_id, "fan", "alarm", id, sensor_type_fan[sensor_fan_id]);
    }
}

int get_next(int sensor_id, const int *sensor_lost){
    int new_sensor_id;
    new_sensor_id = sensor_id;
    do {
        new_sensor_id++;
        if (new_sensor_id >= MAX_SENSOR) {
            new_sensor_id = 0;
        }
        if (sensor_lost[new_sensor_id]){
            return new_sensor_id;
        }
    } while(sensor_id != new_sensor_id);
    return SENSOR_ERROR;
}

int count_sensor_probes(int sensor_id, int screen_id, _Bool sensor_type) {
    int id;
    int count = 0;

    for (id = 1; id <= NUM_PROBES; id++) {
        int val;

        if (screen_id == SCREEN_TEMP) {
            val = get_hwmon(sensor_id, "temp", "input", id, sensor_type);
        } else {
            val = get_hwmon(sensor_id, "fan", "input", id, sensor_type);
        }

        if (val == SENSOR_ERROR) {
            break;
        }
        count++;
    }

    return count;
}

int count_nonzero_fan_probes(int sensor_id, _Bool sensor_type) {
    int id;
    int count = 0;

    for (id = 1; id <= NUM_PROBES; id++) {
        int val = get_hwmon(sensor_id, "fan", "input", id, sensor_type);

        if (val == SENSOR_ERROR) {
            break;
        }
        if (val > 0) {
            count++;
        }
    }

    return count;
}

void auto_select_sensor(int screen_id) {
    int sid;
    int best_id = SENSOR_ERROR;
    int best_count = 0;
    _Bool best_type = 0;
    int *target_id;
    _Bool *target_type;
    int *target_lost;
    const char *label;
    const char *preferred_hwmon_name = NULL;
    int best_pref_score = -1;

    if (screen_id == SCREEN_TEMP) {
        if (!have_temp || sensor_temp_forced) {
            return;
        }
        target_id = &sensor_temp_id;
        target_type = sensor_type_temp;
        target_lost = sensor_lost_temp;
        label = "Temperature";
        if (cpu_vendor_amd) {
            preferred_hwmon_name = "k10temp";
        } else if (cpu_vendor_intel) {
            preferred_hwmon_name = "coretemp";
        }
        if (debug_enabled) {
            fprintf(stderr,
                    "[g15stats] temp auto-select: forced=%d preferred_hwmon=%s\n",
                    sensor_temp_forced,
                    preferred_hwmon_name ? preferred_hwmon_name : "none");
        }
    } else {
        if (!have_fan || sensor_fan_forced) {
            return;
        }
        target_id = &sensor_fan_id;
        target_type = sensor_type_fan;
        target_lost = sensor_lost_fan;
        label = "Fan";
        if (debug_enabled) {
            fprintf(stderr,
                    "[g15stats] fan auto-select: forced=%d preferred labels contain cpu/processor/soc\n",
                    sensor_fan_forced);
        }
    }

    for (sid = 0; sid < MAX_SENSOR; sid++) {
        int count_plain = count_sensor_probes(sid, screen_id, 0);
        int count_device = count_sensor_probes(sid, screen_id, 1);
        int local_count = count_plain;
        _Bool local_type = 0;
        int preferred_match = 0;
        int fan_pref_score = 0;
        int fan_nonzero_count = 0;

        if (count_device > local_count) {
            local_count = count_device;
            local_type = 1;
        }

        if (screen_id == SCREEN_FAN && local_count > 0) {
            fan_nonzero_count = count_nonzero_fan_probes(sid, local_type);
            if (!sensor_fan_forced && fan_nonzero_count == 0) {
                if (debug_enabled) {
                    fprintf(stderr,
                            "[g15stats] fan auto-select: sensor id=%d ignored (all discovered fan inputs are 0)\n",
                            sid);
                }
                continue;
            }

            fan_pref_score = count_preferred_fan_labels(sid, local_type);
            if (debug_enabled) {
                fprintf(stderr,
                        "[g15stats] fan auto-select: sensor id=%d probes=%d nonzero=%d preferred_label_hits=%d\n",
                        sid,
                        local_count,
                        fan_nonzero_count,
                        fan_pref_score);
            }
        }

        if (preferred_hwmon_name != NULL && local_count > 0) {
            preferred_match = hwmon_name_matches(sid, 0, preferred_hwmon_name)
                    || hwmon_name_matches(sid, 1, preferred_hwmon_name);
            if (debug_enabled) {
                fprintf(stderr,
                        "[g15stats] temp auto-select: sensor id=%d probes=%d preferred_match=%d\n",
                        sid,
                        local_count,
                        preferred_match);
            }
            if (!preferred_match) {
                continue;
            }
        }

        if (screen_id == SCREEN_FAN && local_count > 0) {
            if (fan_pref_score > best_pref_score
                    || (fan_pref_score == best_pref_score && local_count > best_count)) {
                best_pref_score = fan_pref_score;
                best_count = local_count;
                best_id = sid;
                best_type = local_type;
            }
            continue;
        }

        if (local_count > best_count) {
            best_count = local_count;
            best_id = sid;
            best_type = local_type;
        }
    }

    if (best_id != SENSOR_ERROR && best_count > 0) {
        *target_id = best_id;
        target_type[best_id] = best_type;
        target_lost[best_id] = RETRY_COUNT;
        if (debug_enabled) {
            fprintf(stderr,
                    "[g15stats] auto-selected %s sensor: id=%d probes=%d path=%s\n",
                    label,
                    best_id,
                    best_count,
                    best_type ? "device" : "plain");
            if (screen_id == SCREEN_TEMP) {
                if (preferred_hwmon_name != NULL) {
                    fprintf(stderr,
                            "[g15stats] temp auto-select result: using preferred hwmon '%s' when available\n",
                            preferred_hwmon_name);
                } else {
                    fprintf(stderr,
                            "[g15stats] temp auto-select result: using generic best-probe candidate\n");
                }
            }
        }
    }
}

int get_sensors(g15_stats_info *sensors, int screen_id, _Bool *sensor_type, int *sensor_lost, int sensor_id) {
    char label[16];
    int count       = 0;
    float amd_temp_pref_cur = -1.0f;
    int amd_temp_pref_found = 0;

    if (screen_id == SCREEN_TEMP) {
        temp_tot_cur    = 0;
        sprintf(label, "Temperature");
    } else { //SCREEN_FAN
        fan_tot_cur     = 0;
        sprintf(label, "Fan");
    }

    for (count = 0; count < NUM_PROBES; count++) {
        if ((sensors[count].cur = get_sensor_cur(count + 1, screen_id)) == SENSOR_ERROR) {
            break;
        }
        sensors[count].max = get_sensor_max(count + 1, screen_id);

        if (screen_id == SCREEN_TEMP) {
            char temp_label[64];

            sensors[count].cur /= 1000;
            sensors[count].max /= 1000;
            if (temp_tot_max < sensors[count].max) {
                temp_tot_max = sensors[count].max;
            }

            if (cpu_vendor_amd && sensor_temp_main == 0) {
                if (get_hwmon_text(sensor_id,
                                   "temp",
                                   "label",
                                   count + 1,
                                   sensor_type[sensor_id],
                                   temp_label,
                                   sizeof(temp_label)) == 0) {
                    if (is_amd_preferred_temp_label(temp_label)
                            && sensors[count].cur > amd_temp_pref_cur) {
                        amd_temp_pref_cur = sensors[count].cur;
                        amd_temp_pref_found = 1;
                    }
                }
            }

            if ((sensor_temp_main == (count +1)) || ((!sensor_temp_main) && (temp_tot_cur < sensors[count].cur))) {
                temp_tot_cur = sensors[count].cur;
            }
        } else { //SCREEN_FAN
            if (fan_tot_cur <sensors[count].cur) {
                fan_tot_cur = sensors[count].cur;
            }

            if (fan_tot_max < fan_tot_cur) {
                fan_tot_max = (fan_tot_cur * 1.2);
            }
        }
    }

    if (screen_id == SCREEN_TEMP && cpu_vendor_amd && sensor_temp_main == 0 && amd_temp_pref_found) {
        temp_tot_cur = amd_temp_pref_cur;
        if (debug_enabled && !debug_cpu_temp_logged) {
            fprintf(stderr,
                    "[g15stats] AMD temp summary auto: using max(Tctl,Tccd*) = %.1f C\n",
                    temp_tot_cur);
            debug_cpu_temp_logged = 1;
        }
    }
    if ((!count) && (sensors[0].cur == SENSOR_ERROR)) {
        if (sensor_type[sensor_id]) {
            sensor_lost[sensor_id]--;
            if(sensor_lost[sensor_id] < 0) {
                sensor_lost[sensor_id] = 0;
            }
            printf("%s sensor doesn't appear to exist with id=%d .\n", label, sensor_id);
            sensor_id = get_next(sensor_id, sensor_lost);
            if (screen_id == SCREEN_TEMP) {
                sensor_temp_id = sensor_id;
            } else { //SCREEN_FAN
                sensor_fan_id = sensor_id;
            }
            if (sensor_id != SENSOR_ERROR) {
                return get_sensors(sensors, screen_id, sensor_type, sensor_lost, sensor_id);
            }
        } else {
            sensor_type[sensor_id] = 1;
            return get_sensors(sensors, screen_id, sensor_type, sensor_lost, sensor_id);
        }

        if (sensor_id == SENSOR_ERROR) {
            if (screen_id == SCREEN_TEMP) {
                have_temp = 0;
            } else { //SCREEN_FAN
                have_fan = 0;
            }
            printf("%s sensor doesn't appear to exist. %s screen will be disabled.\n", label, label);
            return 0;
        }
    }

    if (count >= NUM_PROBES) {
        count = NUM_PROBES;
    } else if(sensors[count].cur != SENSOR_ERROR) {
        count++;
    }
    return count;
}

void print_sys_load_info(g15canvas *canvas, char *tmpstr) {
    glibtop_loadavg loadavg;
    glibtop_uptime uptime;

    glibtop_get_loadavg(&loadavg);
    glibtop_get_uptime(&uptime);
    
    float minutes = uptime.uptime/60;
    float hours = minutes/60;
    float days = 0.0;

    if(hours>24)
        days=(int)(hours/24);
    if(days)
        hours=(int)hours-(days*24);

    sprintf(tmpstr,"LoadAVG %.2f %.2f %.2f | Uptime %.fd%.fh",loadavg.loadavg[0],loadavg.loadavg[1],loadavg.loadavg[2],days,hours);
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_SMALL, 80-(strlen(tmpstr)*4)/2, INFO_ROW);
}

void print_mem_info(g15canvas *canvas, char *tmpstr) {
    glibtop_mem mem;
    glibtop_get_mem(&mem);

    sprintf(tmpstr,"Memory Used %uMB | Memory Total %uMB",(unsigned int)((mem.buffer+mem.cached+mem.user)/(1024*1024)),(unsigned int)(mem.total/(1024*1024)));
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_SMALL, 80-(strlen(tmpstr)*4)/2, INFO_ROW);
}

void print_swap_info(g15canvas *canvas, char *tmpstr) {
    glibtop_swap swap;
    glibtop_get_swap(&swap);

    sprintf(tmpstr,"Swap Used %uMB | Swap Avail. %uMB",(unsigned int)(swap.used/(1024*1024)),(unsigned int)(swap.total/(1024*1024)));
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_SMALL, 80-(strlen(tmpstr)*4)/2, INFO_ROW);
}

void print_net_peak_info(g15canvas *canvas, char *tmpstr) {
    snprintf(tmpstr, MAX_LINES, "Peak IN %s/s|", show_bytes(net_max_in));
    append_textf(tmpstr, MAX_LINES, "Peak OUT %s/s", show_bytes(net_max_out));
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_SMALL, 80-(strlen(tmpstr)*4)/2, INFO_ROW);
}

void print_net_current_info(g15canvas *canvas, char *tmpstr) {
    snprintf(tmpstr, MAX_LINES, "Current IN %s/s|", show_bytes(net_cur_in));
    append_textf(tmpstr, MAX_LINES, "Current OUT %s/s", show_bytes(net_cur_out));
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_SMALL, 80-(strlen(tmpstr)*4)/2, INFO_ROW);
}

void print_freq_info(g15canvas *canvas, char *tmpstr) {
    char proc[24];
    int core;
    int printed = 0;

    init_cpu_count();
    tmpstr[0] = '\0';

    if (ncpu > 6) {
        for (core = 0; core < 3; core++) {
            snprintf(proc, sizeof(proc), "C%d %s", core, show_hertz_short(get_cpu_freq_cur(core)));
            if (printed > 0) {
                append_text(tmpstr, MAX_LINES, "|");
            }
            append_text(tmpstr, MAX_LINES, proc);
            printed++;
        }

        append_text(tmpstr, MAX_LINES, "|...|");

        for (core = ncpu - 3; core < ncpu; core++) {
            snprintf(proc, sizeof(proc), "C%d %s", core, show_hertz_short(get_cpu_freq_cur(core)));
            append_text(tmpstr, MAX_LINES, proc);
            if ((core + 1) < ncpu) {
                append_text(tmpstr, MAX_LINES, "|");
            }
        }
    } else {
        for (core = 0; core < ncpu; core++) {
            snprintf(proc, sizeof(proc), "C%d ", core);
            if (printed > 0) {
                append_text(tmpstr, MAX_LINES, "|");
            }
            append_text(tmpstr, MAX_LINES, proc);
            if (ncpu < 4) {
                append_text(tmpstr, MAX_LINES, show_hertz(get_cpu_freq_cur(core)));
            } else {
                append_text(tmpstr, MAX_LINES, show_hertz_short(get_cpu_freq_cur(core)));
            }
            printed++;
        }
    }

    g15r_renderString(canvas, (unsigned char*) tmpstr, 0, G15_TEXT_SMALL, 80 - (strlen(tmpstr)*4) / 2, INFO_ROW);
}

void print_time_info(g15canvas *canvas, char *tmpstr){
    time_t now;
    time(&now);

    sprintf(tmpstr,"%s",ctime(&now));
    tmpstr[(strlen(tmpstr) - 1)] = '\0';
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_SMALL, 80-(strlen(tmpstr)*4)/2, INFO_ROW);
}

void draw_mem_screen(g15canvas *canvas, char *tmpstr) {
    glibtop_mem mem;

    glibtop_get_mem(&mem);

    int mem_total   = mem.total / 1024;
    int mem_free    = mem.free / 1024;
    int mem_user    = mem.user / 1024;
    int mem_buffer  = mem.buffer / 1024;
    int mem_cached  = mem.cached / 1024;

    g15r_clearScreen (canvas, G15_COLOR_WHITE);

    sprintf(tmpstr,"Usr %2.f%%",((float)mem_user/(float)mem_total)*100);
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, TEXT_LEFT, 2);
    sprintf(tmpstr,"Buf %2.f%%",((float)mem_buffer/(float)mem_total)*100);
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, TEXT_LEFT, 14);
    sprintf(tmpstr,"Che %2.f%%",((float)mem_cached/(float)mem_total)*100);
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, TEXT_LEFT, 26);

    g15r_drawBar(canvas,BAR_START,1,BAR_END,10,G15_COLOR_BLACK,mem_user,mem_total,4);
    g15r_drawBar(canvas,BAR_START,12,BAR_END,21,G15_COLOR_BLACK,mem_buffer,mem_total,4);
    g15r_drawBar(canvas,BAR_START,23,BAR_END,BAR_BOTTOM,G15_COLOR_BLACK,mem_cached,mem_total,4);
    drawBar_reversed(canvas,BAR_START,1,BAR_END,BAR_BOTTOM,G15_COLOR_BLACK,mem_free,mem_total,5);

    drawLine_both(canvas, 1, BAR_BOTTOM);

    print_vert_label(canvas, "FREE");
}

void draw_swap_screen(g15canvas *canvas, char *tmpstr) {
    glibtop_swap swap;

    g15r_clearScreen (canvas, G15_COLOR_WHITE);

    glibtop_get_swap(&swap);

    int swap_used   = swap.used / 1024;
    int swap_total   = swap.total / 1024;

    g15r_renderString (canvas, (unsigned char*)"Swap", 0, G15_TEXT_MED, TEXT_LEFT, 9);
    sprintf(tmpstr,"Used %u%%",(unsigned int)(((float)swap_used/(float)swap_total)*100));
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, TEXT_LEFT, 19);

    drawAll_both(canvas, 1, BAR_BOTTOM, swap_used, swap_total, swap_total-swap_used, swap_total);

    print_vert_label(canvas, "FREE");
}

void print_label(g15canvas *canvas, char *tmpstr, int cur_shift) {
    g15r_renderString(canvas, (unsigned char*) tmpstr, 0, G15_TEXT_MED, TEXT_LEFT, cur_shift + 1);
}

void draw_summary_sensors_logic(g15canvas *canvas, char *tmpstr, const g15_stats_info *sensors,
        const char *label, int text_shift, int y1, int y2, int move, int cur_shift, int shift, int count, float tot_cur, float tot_max) {

    if (count) {
        int j = 0;
        int rest, step;
        step = y2 / count;
        rest = y2 - (step * count);
        int y = cur_shift + y1;
        for (j = 0; j < count; j++) {
            int last_y;
            last_y = y;
            if( j ) {
                last_y++;
                if (rest > 0) {
                    if ((j+1) < count) {
                        y++;
                        rest--;
                    } else {
                        y += rest;
                        rest = 0;
                    }
                }
            }
            y += step;
            drawBar_both(canvas, last_y + move, y + move, sensors[j].cur + 1, tot_max, tot_max - sensors[j].cur, tot_max);

        }
        drawLine_both(canvas, cur_shift + y1 + move, y + move);

        sprintf(tmpstr, label, tot_cur);
        print_label(canvas, tmpstr, text_shift);
    }
}

int sensor_values_have_nonzero(const g15_stats_info *sensors, int count) {
    int i;

    for (i = 0; i < count; i++) {
        if (sensors[i].cur > 0) {
            return 1;
        }
    }

    return 0;
}

void draw_summary_screen(g15canvas *canvas, char *tmpstr, int y1, int y2, int move, int shift, int text_shift, int id) {
    // Memory section
    glibtop_mem mem;
    glibtop_get_mem(&mem);

    int mem_total = (mem.total / 1024);
    int mem_used = mem_total - (mem.free / 1024);

    int cur_shift = shift * id;

    if (summary_rows > 4){
        if (shift > 7) {
            if (id == 2) {
                y2      = 4;
                shift   = 6;
            } else {
                y2      = 5;
                shift   = 7;
            }
        }
    }

    // Memory section
    sprintf(tmpstr, "MEM %3.f%%", ((float) (mem_used) / (float) mem_total)*100);
    print_label(canvas, tmpstr, text_shift * id);

    drawAll_both(canvas, cur_shift + y1 + move, cur_shift + y2 + move, mem_used + 1, mem_total, mem_total - mem_used, mem_total);

    id++;
    cur_shift += shift;

    // Network section
    if (have_nic) {
        int y;

        y = y2 / 2;

        drawLine_both(canvas, cur_shift + y1 + move, cur_shift + y2 + move);

        drawBar_both(canvas, cur_shift + y1 + move, cur_shift + y + move, net_cur_in + 1, net_max_in, net_max_in - net_cur_in, net_max_in);
        drawBar_both(canvas, cur_shift + y + move + 1, cur_shift + y2 + move, net_cur_out + 1, net_max_out, net_max_out - net_cur_out, net_max_out);

        if (net_cur_in > net_cur_out) {
            sprintf(tmpstr, "IN %s", show_bytes_short((int) net_cur_in));
        } else {
            sprintf(tmpstr, "OUT%s", show_bytes_short((int) net_cur_out));
        }
        print_label(canvas, tmpstr, text_shift * id);

    }
    if ((have_temp) || (have_fan)) {
        g15_stats_info sensors[NUM_PROBES];
        memset(sensors, 0, sizeof(sensors));
        // Temperature section
        if ((have_temp) && (id < summary_rows)) {
            int count;

            count = get_sensors(sensors, SCREEN_TEMP, sensor_type_temp, sensor_lost_temp, sensor_temp_id);
            if ((count) && (have_temp)) {
                draw_summary_sensors_logic(canvas, tmpstr, sensors, "TEM %3.f\xb0", text_shift * id, y1, y2, move, cur_shift, shift, count, temp_tot_cur, temp_tot_max);
                id++;
                cur_shift += shift;
            }
        }

        // Fan section
        if ((have_fan) && (id < summary_rows)) {
            int count;

            count = get_sensors(sensors, SCREEN_FAN, sensor_type_fan, sensor_lost_fan, sensor_fan_id);
            if (!sensor_fan_forced && !sensor_values_have_nonzero(sensors, count)) {
                count = 0;
            }
            if ((count) && (have_fan)) {
                draw_summary_sensors_logic(canvas, tmpstr, sensors, "RPM%5.f", text_shift * id, y1, y2, move, cur_shift, shift, count, fan_tot_cur, fan_tot_max);
                id++;
                cur_shift += shift;
            }
        }
    }

    // Swap section
    if (id < summary_rows) {
        glibtop_swap swap;

        glibtop_get_swap(&swap);

        int swap_used = swap.used / 1024;
        int swap_total = swap.total / 1024;

        drawAll_both(canvas, cur_shift + y1 + move, cur_shift + y2 + move, swap_used, swap_total, swap_total - swap_used, swap_total);

        sprintf(tmpstr, "Swp %3u%%", (unsigned int) (((float) swap_used / (float) swap_total)*100));
        print_label(canvas, tmpstr, text_shift * id);

    }
}
/* draw cpu screen.  if drawgraph = 0 then no graph is drawn */
void draw_cpu_screen_unicore_logic(g15canvas *canvas, glibtop_cpu cpu, char *tmpstr, int drawgraph, int printlabels, int cpuandmemory) {
    int total,user,nice,sys,idle;
    static int last_total,last_user,last_nice,last_sys,last_idle,
            b_total,b_user,b_nice,b_sys,b_idle;

    g15r_clearScreen (canvas, G15_COLOR_WHITE);

    total = ((unsigned long) cpu.total) ? ((double) cpu.total) : 1.0;
    user  = ((unsigned long) cpu.user)  ? ((double) cpu.user)  : 1.0;
    nice  = ((unsigned long) cpu.nice)  ? ((double) cpu.nice)  : 1.0;
    sys   = ((unsigned long) cpu.sys)   ? ((double) cpu.sys)   : 1.0;
    idle  = ((unsigned long) cpu.idle)  ? ((double) cpu.idle)  : 1.0;

    if ((total - last_total) > 0) {
        b_total = total - last_total;
        b_user  = user  - last_user;
        b_nice  = nice  - last_nice;
        b_sys   = sys   - last_sys;
        b_idle  = idle  - last_idle;
        last_total  = total;
        last_user   = user;
        last_nice   = nice;
        last_sys    = sys;
        last_idle   = idle;
    } else if (b_total == 0) {
        b_total = 100;
        b_idle  = 100;
    }

    if(printlabels) {
        sprintf(tmpstr,"Usr %2.f%%",((float)b_user/(float)b_total)*100);
        g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 2);
        sprintf(tmpstr,"Sys %2.f%%",((float)b_sys/(float)b_total)*100);
        g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 14);
        sprintf(tmpstr,"Nce %2.f%%",((float)b_nice/(float)b_total)*100);
        g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 26);
    }
    if(drawgraph) {
        g15r_drawBar(canvas,BAR_START,1,BAR_END,10,G15_COLOR_BLACK,b_user+1,b_total,4);
        g15r_drawBar(canvas,BAR_START,12,BAR_END,21,G15_COLOR_BLACK,b_sys+1,b_total,4);
        g15r_drawBar(canvas,BAR_START,23,BAR_END,BAR_BOTTOM,G15_COLOR_BLACK,b_nice+1,b_total,4);
        drawBar_reversed(canvas,BAR_START,1,BAR_END,BAR_BOTTOM,G15_COLOR_BLACK,b_idle+1,b_total,5);

        drawLine_both(canvas, 1, BAR_BOTTOM);
    }
    if (cpuandmemory) {
        print_vert_label(canvas, "TOTAL");

        sprintf(tmpstr,"CPU %3.f%%",((float)(b_total-b_idle)/(float)b_total)*100);
        print_label(canvas, tmpstr, 0);
    } else if ((cycle == SCREEN_FREQ_AGG) && (mode[SCREEN_FREQ_AGG]) && (have_freq)) {
        print_vert_label(canvas, "FREQ");
    } else {
        print_vert_label(canvas, "Idle");
    }
}

void draw_cpu_screen_unicore(g15canvas *canvas, char *tmpstr, int drawgraph, int printlabels) {
    glibtop_cpu cpu;
    glibtop_get_cpu(&cpu);

    draw_cpu_screen_unicore_logic(canvas, cpu, tmpstr, drawgraph, printlabels, 0);
}

void draw_freq_screen_aggregate(g15canvas *canvas, char *tmpstr) {
    int core;
    int i;
    int group_size = 1;
    int group_count;
    int max_bars_fit = (BAR_END - BAR_START + 1) / 3;
    int bar_top = 1;
    int bar_bottom = G15_LCD_HEIGHT - 1;
    int bar_height = bar_bottom - bar_top + 1;
    int global_max = 0;
    int global_min = 0;
    int global_sum = 0;
    int global_count = 0;
    int global_min_allowed = 0;
    int baseline = 0;
    static int last_logged_baseline = -1;
    static int last_logged_group_count = -1;
    static int last_logged_bar_width = -1;

    if (!have_freq) {
        return;
    }

    g15r_clearScreen(canvas, G15_COLOR_WHITE);

    init_cpu_count();

    if (max_bars_fit < 1) {
        max_bars_fit = 1;
    }

    if (ncpu <= 0) {
        return;
    }

    while (((ncpu + group_size - 1) / group_size) > max_bars_fit) {
        group_size++;
    }

    group_count = (ncpu + group_size - 1) / group_size;

    if (debug_enabled) {
        int bar_width;

        bar_width = (BAR_END - BAR_START + 1) / group_count;
        if (bar_width != last_logged_bar_width || group_count != last_logged_group_count) {
            fprintf(stderr,
                    "[g15stats] freq agg layout: bars=%d, bar_width=%d px, group_size=%d\n",
                    group_count,
                    bar_width,
                    group_size);
            last_logged_bar_width = bar_width;
            last_logged_group_count = group_count;
        }
    }

    for (core = 0; core < ncpu; core++) {
        int freq_cur = get_cpu_freq_cur(core);
        int freq_min = get_cpu_freq_min(core);

        if (freq_cur <= 0) {
            continue;
        }

        if (global_count == 0 || freq_cur > global_max) {
            global_max = freq_cur;
        }
        if (global_count == 0 || freq_cur < global_min) {
            global_min = freq_cur;
        }

        global_sum += freq_cur;
        global_count++;

        if (freq_min > 0 && (global_min_allowed == 0 || freq_min < global_min_allowed)) {
            global_min_allowed = freq_min;
        }
    }

    if (global_count == 0) {
        return;
    }

    int global_scale = global_max;

    if (global_scale <= 0) {
        global_scale = 1;
    }

    if (global_min_allowed > 100000) {
        baseline = global_min_allowed - 100000;
    } else {
        baseline = 0;
    }
    if (global_scale <= baseline) {
        baseline = 0;
    }

    if (debug_enabled && baseline != last_logged_baseline) {
        fprintf(stderr,
                "[g15stats] freq baseline changed: %.1f MHz (min allowed: %.1f MHz, offset: -100.0 MHz)\n",
                ((double) baseline) / 1000.0,
                ((double) global_min_allowed) / 1000.0);
        last_logged_baseline = baseline;
    }

    for (i = 0; i < group_count; i++) {
        int start_core = i * group_size;
        int end_core = start_core + group_size;
        int sum = 0;
        int count = 0;
        int x1;
        int x2;
        int avg;
        int h;
        int bar_value;
        int bar_total;
        int y1;

        if (end_core > ncpu) {
            end_core = ncpu;
        }

        for (core = start_core; core < end_core; core++) {
            int freq_cur = get_cpu_freq_cur(core);
            if (freq_cur > 0) {
                sum += freq_cur;
                count++;
            }
        }

        if (count == 0) {
            continue;
        }

        avg = sum / count;

        x1 = BAR_START + ((i * (BAR_END - BAR_START + 1)) / group_count);
        x2 = BAR_START + ((((i + 1) * (BAR_END - BAR_START + 1)) / group_count) - 1);
        if (x2 > x1) {
            x2--;
        }
        if (x2 < x1) {
            x2 = x1;
        }

        bar_value = avg - baseline;
        bar_total = global_scale - baseline;
        if (bar_total <= 0) {
            bar_total = global_scale;
            bar_value = avg;
        }
        if (bar_value < 0) {
            bar_value = 0;
        }

        h = (bar_value * bar_height) / bar_total;
        if (h < 1) {
            h = 1;
        }
        if (h > bar_height) {
            h = bar_height;
        }

        y1 = bar_bottom - h + 1;
        g15r_pixelBox(canvas, x1, y1, x2, bar_bottom, G15_COLOR_BLACK, 1, 1);
    }

    sprintf(tmpstr, "M:%s", show_hertz_short(global_max));
    g15r_renderString(canvas, (unsigned char*) tmpstr, 0, G15_TEXT_MED, 1, 2);

    sprintf(tmpstr, "A:%s", show_hertz_short(global_sum / global_count));
    g15r_renderString(canvas, (unsigned char*) tmpstr, 0, G15_TEXT_MED, 1, 14);

    sprintf(tmpstr, "L:%s", show_hertz_short(global_min));
    g15r_renderString(canvas, (unsigned char*) tmpstr, 0, G15_TEXT_MED, 1, 26);

    print_vert_label(canvas, "FREQ");
}

static int scaled_bar_size(int value, int total, int max_size) {
    if (max_size <= 0) {
        return 0;
    }
    if (value <= 0 || total <= 0) {
        return 0;
    }

    {
        int scaled = (max_size * value) / total;
        if (scaled < 1) {
            scaled = 1;
        }
        if (scaled > max_size) {
            scaled = max_size;
        }
        return scaled;
    }
}

void draw_cpu_screen_load2(g15canvas *canvas, glibtop_cpu cpu, char *tmpstr) {
    static int last_total[GLIBTOP_NCPU];
    static int last_user[GLIBTOP_NCPU];
    static int last_nice[GLIBTOP_NCPU];
    static int last_sys[GLIBTOP_NCPU];
    static int last_idle[GLIBTOP_NCPU];
    int d_total[GLIBTOP_NCPU];
    int d_user[GLIBTOP_NCPU];
    int d_nice[GLIBTOP_NCPU];
    int d_sys[GLIBTOP_NCPU];
    int d_idle[GLIBTOP_NCPU];
    int core;
    int drawable_height = BAR_BOTTOM + 1;
    int line_count = drawable_height / cpu2_bar_height;
    int grouped_lines;
    int bar_height;
    int y_offset;
    int line;
    int total_user = 0;
    int total_sys = 0;
    int total_nice = 0;
    int total_total = 0;

    g15r_clearScreen(canvas, G15_COLOR_WHITE);

    if (ncpu <= 0) {
        return;
    }

    if (line_count < 1) {
        line_count = 1;
    }

    if (ncpu > line_count) {
        grouped_lines = ncpu / 2;
        if (grouped_lines < 1) {
            grouped_lines = 1;
        }
        if (grouped_lines < line_count) {
            line_count = grouped_lines;
        }
    }

    if (line_count > ncpu) {
        line_count = ncpu;
    }

    bar_height = drawable_height / line_count;
    if (bar_height < 1) {
        bar_height = 1;
    }
    y_offset = (drawable_height - (bar_height * line_count)) / 2;

    if (debug_enabled && !cpu2_debug_logged) {
        fprintf(stderr,
                "[g15stats] cpu2 layout: cores=%d bars=%d bar_h=%d\n",
                ncpu,
                line_count,
                bar_height);
    }

    for (core = 0; core < ncpu; core++) {
        int total = ((unsigned long) cpu.xcpu_total[core]) ? ((double) cpu.xcpu_total[core]) : 1.0;
        int user = ((unsigned long) cpu.xcpu_user[core]) ? ((double) cpu.xcpu_user[core]) : 1.0;
        int nice = ((unsigned long) cpu.xcpu_nice[core]) ? ((double) cpu.xcpu_nice[core]) : 1.0;
        int sys = ((unsigned long) cpu.xcpu_sys[core]) ? ((double) cpu.xcpu_sys[core]) : 1.0;
        int idle = ((unsigned long) cpu.xcpu_idle[core]) ? ((double) cpu.xcpu_idle[core]) : 1.0;

        if ((total - last_total[core]) > 0) {
            d_total[core] = total - last_total[core];
            d_user[core] = user - last_user[core];
            d_nice[core] = nice - last_nice[core];
            d_sys[core] = sys - last_sys[core];
            d_idle[core] = idle - last_idle[core];
            if (d_user[core] < 0) d_user[core] = 0;
            if (d_nice[core] < 0) d_nice[core] = 0;
            if (d_sys[core] < 0) d_sys[core] = 0;
            if (d_idle[core] < 0) d_idle[core] = 0;
        } else {
            d_total[core] = 100;
            d_user[core] = 0;
            d_nice[core] = 0;
            d_sys[core] = 0;
            d_idle[core] = 100;
        }

        last_total[core] = total;
        last_user[core] = user;
        last_nice[core] = nice;
        last_sys[core] = sys;
        last_idle[core] = idle;

        total_total += d_total[core];
        total_user += d_user[core];
        total_sys += d_sys[core];
        total_nice += d_nice[core];
    }

    if (total_total <= 0) {
        total_total = 1;
    }

    snprintf(tmpstr, MAX_LINES, "Usr %4.1f%%", ((float) total_user * 100.0f) / (float) total_total);
    g15r_renderString(canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 2);
    snprintf(tmpstr, MAX_LINES, "Sys %4.1f%%", ((float) total_sys * 100.0f) / (float) total_total);
    g15r_renderString(canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 14);
    snprintf(tmpstr, MAX_LINES, "Nce %4.1f%%", ((float) total_nice * 100.0f) / (float) total_total);
    g15r_renderString(canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 26);

    for (line = 0; line < line_count; line++) {
        int start_core = (line * ncpu) / line_count;
        int end_core = ((line + 1) * ncpu) / line_count;
        int y1 = y_offset + (line * bar_height);
        int y2 = y1 + bar_height - 1;
        int bar_h = bar_height;
        int sum_total = 0;
        int sum_active = 0;
        int sum_sys = 0;
        int sum_nice = 0;
        int bar_w;
        int x1;
        int x2;

        if (end_core <= start_core) {
            end_core = start_core + 1;
            if (end_core > ncpu) {
                end_core = ncpu;
            }
        }

        for (core = start_core; core < end_core; core++) {
            int total = d_total[core];
            int nice = d_nice[core];
            int sys = d_sys[core];
            int idle = d_idle[core];
            int active;

            if (total <= 0) {
                total = 100;
                idle = 100;
                nice = 0;
                sys = 0;
            }

            active = total - idle;
            if (active < 0) {
                active = 0;
            }

            sum_total += total;
            sum_active += active;
            sum_sys += sys;
            sum_nice += nice;
        }

        if (sum_total <= 0 || sum_active <= 0) {
            continue;
        }

        bar_w = scaled_bar_size(sum_active,
                                sum_total,
                                (BAR_END - BAR_START + 1));
        if (bar_w > 0 && bar_w < cpu2_min_bar_width) {
            bar_w = cpu2_min_bar_width;
        }
        if (bar_w > (BAR_END - BAR_START + 1)) {
            bar_w = (BAR_END - BAR_START + 1);
        }

        if (debug_enabled && !cpu2_debug_logged) {
            fprintf(stderr,
                    "[g15stats] cpu2 bar: line=%d cores=%d-%d width=%dpx height=%dpx load=%d/%d\n",
                    line,
                    start_core,
                    end_core - 1,
                    bar_w,
                    bar_h,
                    sum_active,
                    sum_total);
        }

        x1 = BAR_START;
        x2 = BAR_START + bar_w - 1;
        if (bar_chart_background) {
            g15r_pixelBox(canvas, BAR_START, y1, BAR_END, y2, G15_COLOR_BLACK, 1, 0);
        }
        g15r_pixelBox(canvas, x1, y1, x2, y2, G15_COLOR_BLACK, 1, 1);

        if (bar_h >= 2) {
            int nice_w = (bar_w * sum_nice) / sum_active;
            int nx1;
            int x;

            if (nice_w > bar_w) {
                nice_w = bar_w;
            }
            if (nice_w > 0) {
                nx1 = x2 - nice_w + 1;
                for (x = nx1; x <= x2; x++) {
                    if ((x & 1) == 0) {
                        g15r_setPixel(canvas, x, y2, G15_COLOR_WHITE);
                    }
                }
            }
        }

        if (bar_h >= 3) {
            int sys_w = (bar_w * sum_sys) / sum_active;
            int sx1;
            int x;

            if (sys_w > bar_w) {
                sys_w = bar_w;
            }
            if (sys_w > 0) {
                sx1 = x2 - sys_w + 1;
                for (x = sx1; x <= x2; x++) {
                    if ((x & 1) != 0) {
                        g15r_setPixel(canvas, x, y2 - 1, G15_COLOR_WHITE);
                    }
                    if ((x & 1) == 0) {
                        g15r_setPixel(canvas, x, y2 - 2, G15_COLOR_WHITE);
                    }
                }
            }
        }
    }

    if (debug_enabled && !cpu2_debug_logged) {
        cpu2_debug_logged = 1;
    }
}

void draw_cpu_screen_vertical(g15canvas *canvas, glibtop_cpu cpu, char *tmpstr) {
    static int last_total[GLIBTOP_NCPU];
    static int last_nice[GLIBTOP_NCPU];
    static int last_sys[GLIBTOP_NCPU];
    static int last_idle[GLIBTOP_NCPU];
    int d_total[GLIBTOP_NCPU];
    int d_nice[GLIBTOP_NCPU];
    int d_sys[GLIBTOP_NCPU];
    int d_idle[GLIBTOP_NCPU];
    int core;
    int groups;
    int gap = 1;
    int total_user = 0;
    int total_sys = 0;
    int total_nice = 0;
    int total_total = 0;
    int usable_w = BAR_END - BAR_START + 1;
    int max_groups = usable_w / 3;

    g15r_clearScreen(canvas, G15_COLOR_WHITE);

    if (ncpu <= 0) {
        return;
    }

    for (core = 0; core < ncpu; core++) {
        int total = ((unsigned long) cpu.xcpu_total[core]) ? ((double) cpu.xcpu_total[core]) : 1.0;
        int nice = ((unsigned long) cpu.xcpu_nice[core]) ? ((double) cpu.xcpu_nice[core]) : 1.0;
        int sys = ((unsigned long) cpu.xcpu_sys[core]) ? ((double) cpu.xcpu_sys[core]) : 1.0;
        int idle = ((unsigned long) cpu.xcpu_idle[core]) ? ((double) cpu.xcpu_idle[core]) : 1.0;

        if ((total - last_total[core]) > 0) {
            d_total[core] = total - last_total[core];
            d_sys[core] = sys - last_sys[core];
            d_nice[core] = nice - last_nice[core];
            d_idle[core] = idle - last_idle[core];
            if (d_sys[core] < 0) d_sys[core] = 0;
            if (d_nice[core] < 0) d_nice[core] = 0;
            if (d_idle[core] < 0) d_idle[core] = 0;
        } else {
            d_total[core] = 100;
            d_sys[core] = 0;
            d_nice[core] = 0;
            d_idle[core] = 100;
        }

        last_total[core] = total;
        last_nice[core] = nice;
        last_sys[core] = sys;
        last_idle[core] = idle;

        total_total += d_total[core];
        total_user += (d_total[core] - d_idle[core] - d_sys[core] - d_nice[core]);
        total_sys += d_sys[core];
        total_nice += d_nice[core];
    }

    if (total_total <= 0) {
        total_total = 1;
    }

    snprintf(tmpstr, MAX_LINES, "Usr %4.1f%%", ((float) total_user * 100.0f) / (float) total_total);
    g15r_renderString(canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 2);
    snprintf(tmpstr, MAX_LINES, "Sys %4.1f%%", ((float) total_sys * 100.0f) / (float) total_total);
    g15r_renderString(canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 14);
    snprintf(tmpstr, MAX_LINES, "Nce %4.1f%%", ((float) total_nice * 100.0f) / (float) total_total);
    g15r_renderString(canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 26);

    if (max_groups < 1) {
        max_groups = 1;
    }
    groups = ncpu;
    if (groups > max_groups) {
        groups = max_groups;
    }

    for (core = 0; core < groups; core++) {
        int start_core = (core * ncpu) / groups;
        int end_core = ((core + 1) * ncpu) / groups;
        int x1;
        int x2;
        int y2 = BAR_BOTTOM;
        int bar_h;
        int sum_total = 0;
        int sum_active = 0;
        int sum_sys = 0;
        int sum_nice = 0;
        int c;

        if (end_core <= start_core) {
            end_core = start_core + 1;
            if (end_core > ncpu) {
                end_core = ncpu;
            }
        }

        x1 = BAR_START + (core * usable_w) / groups;
        x2 = BAR_START + (((core + 1) * usable_w) / groups) - 1 - gap;
        if (x2 < x1) {
            x2 = x1;
        }

        for (c = start_core; c < end_core; c++) {
            int active = d_total[c] - d_idle[c];
            if (active < 0) {
                active = 0;
            }
            sum_total += d_total[c];
            sum_active += active;
            sum_sys += d_sys[c];
            sum_nice += d_nice[c];
        }

        if (sum_total <= 0 || sum_active <= 0) {
            continue;
        }

        bar_h = scaled_bar_size(sum_active,
                                sum_total,
                                (BAR_BOTTOM + 1));

        if (bar_chart_background) {
            g15r_pixelBox(canvas, x1, 0, x2, BAR_BOTTOM, G15_COLOR_BLACK, 1, 0);
        }
        g15r_pixelBox(canvas, x1, y2 - bar_h + 1, x2, y2, G15_COLOR_BLACK, 1, 1);

        if ((x2 - x1 + 1) >= 2) {
            int nice_h = (bar_h * sum_nice) / sum_active;
            int y;
            if (nice_h > bar_h) {
                nice_h = bar_h;
            }
            for (y = y2; y > y2 - nice_h; y--) {
                int x;
                for (x = x1; x <= x2; x++) {
                    if ((x + y) & 1) {
                        g15r_setPixel(canvas, x, y, G15_COLOR_WHITE);
                    }
                }
            }
        }
        if ((x2 - x1 + 1) >= 3) {
            int sys_h = (bar_h * sum_sys) / sum_active;
            int y;
            if (sys_h > bar_h) {
                sys_h = bar_h;
            }
            for (y = y2 - 1; y > y2 - 1 - sys_h; y--) {
                if (y < 0) {
                    break;
                }
                if ((x1 + 1) <= x2) {
                    g15r_setPixel(canvas, x1 + 1, y, G15_COLOR_WHITE);
                }
                if ((x2 - 1) >= x1) {
                    g15r_setPixel(canvas, x2 - 1, y, G15_COLOR_WHITE);
                }
            }
        }
    }
}

void draw_cpu_screen_multicore(g15canvas *canvas, char *tmpstr, int unicore) {
    glibtop_cpu cpu;
    int core,ncpumax;
    int divider = 0;
        
    int total,user,nice,sys,idle;
    int sub_val;
    static int last_total[GLIBTOP_NCPU],last_user[GLIBTOP_NCPU],last_nice[GLIBTOP_NCPU],
            last_sys[GLIBTOP_NCPU],last_idle[GLIBTOP_NCPU],
            b_total[GLIBTOP_NCPU],b_user[GLIBTOP_NCPU],
            b_nice[GLIBTOP_NCPU],b_sys[GLIBTOP_NCPU],b_idle[GLIBTOP_NCPU];

    init_cpu_count();

    ncpumax = ncpu;

    glibtop_get_cpu(&cpu);

    switch (cycle) {
        case    SCREEN_SUMMARY  :
            draw_cpu_screen_unicore_logic(canvas, cpu, tmpstr, 0, 0, 1);
            break;
        case SCREEN_CPU :
            if ((unicore) || (ncpu==1)) {
               draw_cpu_screen_unicore_logic(canvas, cpu, tmpstr, 1, 1, 0);
                return;
            }
            draw_cpu_screen_unicore_logic(canvas, cpu, tmpstr, 0, 1, 0);
            break;
        case SCREEN_CPU2:
            if (mode[SCREEN_CPU2]) {
                draw_cpu_screen_vertical(canvas, cpu, tmpstr);
            } else {
                draw_cpu_screen_load2(canvas, cpu, tmpstr);
            }
            return;
        case SCREEN_FREQ_AGG:
            draw_freq_screen_aggregate(canvas, tmpstr);
            return;
    }

    int y1=0, y2=BAR_BOTTOM;
    int shift = 12;
    int shift2 = 24;
    int current_value=1;
    int freq_cur = 1;
    int freq_sum = 0;
    int summary_bar_end = BAR_START;

    int spacer = 1;
    int height = 9;
    int move   = 0;
    switch (cycle) {
        case    SCREEN_CPU :
            if(ncpu > 4){
                spacer = 0;
            }
            height = 12;
            break;
        case    SCREEN_CPU2 :
            if(ncpu > 4){
                spacer = 0;
            }
            height = 12;
            break;
        case    SCREEN_SUMMARY :
            spacer = 0;
            if (!mode[SCREEN_SUMMARY]) {
                summary_rows = 5;
                switch (ncpu) {
                    case 1  :
                    case 2  :
                    case 5  :
                    case 7  :
                        move    = 1;
                        height  = 6;
                        break;
                    case 3  :
                    case 6  :
                        height  = 6;
                        break;
                    default :
                        height  = 8;
                        break;
                }
            } else {
                summary_rows = 4;
                switch (ncpu) {
                    case 3  :
                    case 5  :
                        move    = 1;
                        break;
                }
                        height  = 8;
                }
                
            shift   = height + 1;
            shift2  = (2 * shift);
            break;
    }

    ncpumax = height;
    if (ncpumax < ncpu) {
        height = (height - ((ncpumax - 1) * spacer)) / (ncpumax);
    } else {
        height = (height - ((ncpu - 1) * spacer)) / (ncpu);
    }

    if (height < 1) {
        height = 1;
    }

    for(core = 0; ((core < ncpu) && (core < ncpumax)); core++) {
        total = ((unsigned long) cpu.xcpu_total[core]) ? ((double) cpu.xcpu_total[core]) : 1.0;
        user  = ((unsigned long) cpu.xcpu_user[core])  ? ((double) cpu.xcpu_user[core])  : 1.0;
        nice  = ((unsigned long) cpu.xcpu_nice[core])  ? ((double) cpu.xcpu_nice[core])  : 1.0;
        sys   = ((unsigned long) cpu.xcpu_sys[core])   ? ((double) cpu.xcpu_sys[core])   : 1.0;
        idle  = ((unsigned long) cpu.xcpu_idle[core])  ? ((double) cpu.xcpu_idle[core])  : 1.0;

        if ((total - last_total[core]) > 0) {
            b_total[core]   = total - last_total[core];
            b_user[core]    = user  - last_user[core];
            b_nice[core]    = nice  - last_nice[core];
            b_sys[core]     = sys   - last_sys[core];
            b_idle[core]    = idle  - last_idle[core];
            last_total[core]	= total;
            last_user[core] 	= user;
            last_nice[core] 	= nice;
            last_sys[core] 	= sys;
            last_idle[core] 	= idle;
        } else if (b_total[core] == 0){
            b_total[core] = 100;
            b_idle[core]  = 100;
        }

        y1 = (core * height) + (core * spacer);
        y2 = y1 + height - 1;

        switch (cycle) {
            case SCREEN_CPU:
                if (mode[cycle]) {
                    divider = 9 / ncpu;
                    sub_val = divider * core;
                    g15r_drawBar(canvas, BAR_START, sub_val, BAR_END, divider + sub_val, G15_COLOR_BLACK, b_user[core] + 1, b_total[core], 4);
                    g15r_drawBar(canvas, BAR_START, shift + sub_val, BAR_END, shift + divider + sub_val, G15_COLOR_BLACK, b_sys[core] + 1, b_total[core], 4);
                    y1 = 0;
                    y2 = shift2 + divider + sub_val;
                    g15r_drawBar(canvas, BAR_START, shift2 + sub_val, BAR_END, y2, G15_COLOR_BLACK, b_nice[core] + 1, b_total[core], 4);

                    divider = y2 / ncpu;
                    drawBar_reversed(canvas, BAR_START, sub_val, BAR_END, y2, G15_COLOR_BLACK, b_idle[core] + 1, b_total[core], 5);
                } else {
                    current_value = b_total[core] - b_idle[core];
                    drawBar_both(canvas, y1, y2, current_value, b_total[core], b_total[core] - current_value, b_total[core]);

                    drawBar_both(canvas, shift + y1, shift + y2, b_sys[core] + 1, b_total[core], b_total[core] - b_sys[core], b_total[core]);

                    y2 += shift2;
                    drawBar_both(canvas, shift2 + y1, y2, b_nice[core] + 1, b_total[core], b_total[core] - b_nice[core], b_total[core]);

                    y1 = 0;
                }
                break;
            case SCREEN_CPU2:
                if (mode[cycle]) {
                    divider = 9 / ncpu;
                    sub_val = divider * core;
                    g15r_drawBar(canvas, BAR_START, sub_val, BAR_END, divider + sub_val, G15_COLOR_BLACK, b_user[core] + 1, b_total[core], 4);
                    g15r_drawBar(canvas, BAR_START, shift + sub_val, BAR_END, shift + divider + sub_val, G15_COLOR_BLACK, b_sys[core] + 1, b_total[core], 4);
                    y1 = 0;
                    y2 = shift2 + divider + sub_val;
                    g15r_drawBar(canvas, BAR_START, shift2 + sub_val, BAR_END, y2, G15_COLOR_BLACK, b_nice[core] + 1, b_total[core], 4);

                    divider = y2 / ncpu;
                    drawBar_reversed(canvas, BAR_START, sub_val, BAR_END, y2, G15_COLOR_BLACK, b_idle[core] + 1, b_total[core], 5);
                } else {
                    current_value = b_total[core] - b_idle[core];
                    drawBar_both(canvas, y1, y2, current_value, b_total[core], b_total[core] - current_value, b_total[core]);

                    drawBar_both(canvas, shift + y1, shift + y2, b_sys[core] + 1, b_total[core], b_total[core] - b_sys[core], b_total[core]);

                    y2 += shift2;
                    drawBar_both(canvas, shift2 + y1, y2, b_nice[core] + 1, b_total[core], b_total[core] - b_nice[core], b_total[core]);

                    y1 = 0;
                }
                break;
            case SCREEN_SUMMARY:
                current_value = b_total[core] - b_idle[core];
                drawBar_both(canvas, y1 + move, y2 + move, current_value, b_total[core], b_total[core] - current_value, b_total[core]);

                if (b_total[core] > 0) {
                    int bar_end = BAR_START + (((BAR_END - BAR_START + 1) * current_value) / b_total[core]);
                    if (bar_end > summary_bar_end) {
                        summary_bar_end = bar_end;
                    }
                }

                if (have_freq) {
                    freq_cur = get_cpu_freq_cur(core);
                    freq_sum = maxi(freq_sum, freq_cur);
                }

                y1 = 0;
                break;
        }
    }

    drawLine_both(canvas, y1 + move, y2 + move);

    if (cycle == SCREEN_SUMMARY) {
        int text_shift;
        int freq_x = 0;
        int freq_y = 1;
        int overlap = 0;

        if (summary_rows > 4) {
            text_shift = 7;
        } else {
            text_shift = 9;
        }

        if (have_freq) {
            float ghz = ((float) freq_sum) / 1000000.0f;

            snprintf(tmpstr, MAX_LINES, "%0.2fGHz", ghz);
            freq_x = G15_LCD_WIDTH - (((int) strlen(tmpstr)) * 6) - 1;
            if (freq_x < BAR_START) {
                freq_x = BAR_START;
            }

            if (summary_bar_end >= freq_x) {
                overlap = 1;
            }

            if (overlap) {
                canvas->mode_xor = 1;
            }
            g15r_renderString(canvas, (unsigned char*) tmpstr, 0, G15_TEXT_MED, freq_x, freq_y);
            if (overlap) {
                canvas->mode_xor = 0;
            }
        }

        draw_summary_screen(canvas, tmpstr, y1, y2, move, shift, text_shift, 1);
    }
}

void draw_net_screen(g15canvas *canvas, char *tmpstr, char *interface) {
    int i;
    int x=0;

    float diff=0;
    float height=0;
    float last=0;

    g15r_clearScreen (canvas, G15_COLOR_WHITE);
    glibtop_netload netload;
    glibtop_get_netload(&netload,interface);    
    // in
    x=53;
    for(i=net_rr_index+1;i<MAX_NET_HIST;i++) {
      diff = (float) net_max_in / (float) net_hist[i][0];
      height = 16-(16/diff);
      g15r_setPixel(canvas,x,height,G15_COLOR_BLACK);
      g15r_drawLine(canvas,x,height,x-1,last,G15_COLOR_BLACK);
      last=height;
      x++;
    }
    for(i=0;i<net_rr_index;i++) {
      diff = (float) net_max_in / (float) net_hist[i][0];
      height = 16-(16 / diff);
      g15r_drawLine(canvas,x,height,x-1,last,G15_COLOR_BLACK);
      last=height;
      x++;
    }
    // out
    x=53;
    for(i=net_rr_index+1;i<MAX_NET_HIST;i++) {
      diff = (float) net_max_out / (float) net_hist[i][1];
      height = 34-(16/diff);
      g15r_setPixel(canvas,x,height,G15_COLOR_BLACK);
      g15r_drawLine(canvas,x,height,x-1,last,G15_COLOR_BLACK);
      last=height;
      x++;
    }
    for(i=0;i<net_rr_index;i++) {
      diff = (float) net_max_out / (float) net_hist[i][1];
      height = 34-(16 / diff);
      g15r_drawLine(canvas,x,height,x-1,last,G15_COLOR_BLACK);
      last=height;
      x++;
    }
    g15r_drawLine (canvas, 52, 0, 52, 34, G15_COLOR_BLACK);
    g15r_drawLine (canvas, 53, 0, 53, 34, G15_COLOR_BLACK);
    g15r_drawLine (canvas, 54, 0, 54, 34, G15_COLOR_BLACK);

    snprintf(tmpstr, MAX_LINES, "IN %s", show_bytes(netload.bytes_in));
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 2);
    snprintf(tmpstr, MAX_LINES, "OUT %s", show_bytes(netload.bytes_out));
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, 26);

    snprintf(tmpstr, MAX_LINES, "%s", interface);
    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_LARGE, 25-(strlen(tmpstr)*9)/2, 14);

    if (net_scale_absolute) {
        print_vert_label_logic(canvas, "ABS  ", 47);
    } else {
        print_vert_label_logic(canvas, "REL  ", 47);
    }
}

void draw_bat_screen(g15canvas *canvas, char *tmpstr, int all) {

	g15_stats_bat_info bats[NUM_BATS];
	long	tot_max_charge = 0;
	long	tot_cur_charge = 0;

	FILE	*fd_state;
	FILE	*fd_info;
	char	line	[MAX_LINES];
	char	value	[MAX_LINES];

	int i = 0;
	for (i = 0; i < NUM_BATS; i++)
	{
		char filename[30];

		// Initialize battery state
		bats[i].max_charge = 0;
		bats[i].cur_charge = 0;
		bats[i].status = -1;

		snprintf(filename, sizeof(filename), "/proc/acpi/battery/BAT%d/state", i);
		fd_state=fopen (filename,"r");
		if (fd_state!=NULL)
		{
			while (fgets (line,MAX_LINES,fd_state)!=NULL)
			{
				// Parse the state file for battery info
				if (strcasestr (line,"remaining capacity")!=0)
				{
					snprintf(value, sizeof(value), "%.5s", line + 25);
					bats[i].cur_charge=atoi (value);
				}
				if (strcasestr (line,"charging state")!=0)
				{
					if (strcasestr (line,"charged")!=0)
					{
						bats[i].status=0;
					}
					if (strcasestr (line," charging")!=0)
					{
						bats[i].status=1;
					}
					if (strcasestr (line,"discharging")!=0)
					{
						bats[i].status=2;
					}
				}
			}
			fclose (fd_state);
			snprintf(filename, sizeof(filename), "/proc/acpi/battery/BAT%d/info", i);
			fd_info=fopen (filename,"r");

			if (fd_info!=NULL)
			{
				while (fgets (line,MAX_LINES,fd_info)!=NULL)
				{
					// Parse the info file for battery info
					if (strcasestr (line,"last full capacity")!=0)
					{
						snprintf(value, sizeof(value), "%.5s", line + 25);
						bats[i].max_charge=atoi (value);
					}
				}
				fclose (fd_info);
			}

			tot_cur_charge += bats[i].cur_charge;
			tot_max_charge += bats[i].max_charge;

		} else {
                    break;
                }
	}

        if (i) {
            sensor_lost_bat = RETRY_COUNT;
        } else {
            sensor_lost_bat--;
            if (sensor_lost_bat <= 0) {
                printf("Battery sensor doesn't appear to exist. Battery screen will be disabled.\n");
                have_bat = 0;
                return;
            }
        }

        if (all) {
            g15r_clearScreen (canvas, G15_COLOR_WHITE);

            print_vert_label(canvas, "FULL");

            drawLine_both(canvas, 1, BAR_BOTTOM);

            for (i = 0; i < NUM_BATS; i++)
            {
                    register float charge = 0;
                    register int bar_top = (i*10) + 1 + i;
                    register int bar_bottom = ((i+1)*10) + i;
                    if (bats[i].max_charge > 0)
                    {
                            charge = ((float)bats[i].cur_charge/(float)bats[i].max_charge)*100;
                    }
                    snprintf(tmpstr, MAX_LINES, "Bt%d %2.f%%", i, charge);
                    g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, 1, (i*12) + 2);
                    g15r_drawBar(canvas, BAR_START, bar_top, BAR_END, bar_bottom, G15_COLOR_BLACK, bats[i].cur_charge, bats[i].max_charge, 4);
            }

            drawBar_reversed(canvas,BAR_START,1,BAR_END,BAR_BOTTOM,G15_COLOR_BLACK,100-(((float)tot_cur_charge/(float)tot_max_charge)*100),100,5);
        }

        if ((!all) || (info_cycle == SCREEN_BAT)) {
            float total_charge = 0;
            if (tot_max_charge > 0)
            {
                    total_charge = ((float)tot_cur_charge/(float)tot_max_charge)*100;
            }
            snprintf(tmpstr, MAX_LINES, "Total %2.f%% | ", total_charge);

            for (i = 0; i < NUM_BATS; i++)
            {
                    char extension[11];
                    switch (bats[i].status)
                    {
                            case -1:
                            {
                                    snprintf(extension, sizeof(extension), "Bt%d - | ", i);
                                    break;
                            }
                            case 0:
                            {
                                    snprintf(extension, sizeof(extension), "Bt%d F | ", i);
                                    break;
                            }
                            case 1:
                            {
                                    snprintf(extension, sizeof(extension), "Bt%d C | ", i);
                                    break;
                            }
                            case 2:
                            {
                                    snprintf(extension, sizeof(extension), "Bt%d D | ", i);
                                    break;
                            }
                    }

                    append_text(tmpstr, MAX_LINES, extension);
            }

            g15r_renderString (canvas, (unsigned char*)tmpstr, 0, G15_TEXT_SMALL, 80-(strlen(tmpstr)*4)/2, INFO_ROW);
        }
}

void draw_g15_stats_info_screen_logic(g15canvas *canvas, char *tmpstr, int all, int screen_type,
        const g15_stats_info *probes, int count, float tot_max, int *sensor_lost, int sensor_id,
        const char *vert_label, const char *format_main, const char *format_bottom) {
    
    if ((!count) || (probes[0].cur == SENSOR_ERROR)) {
        return;
    } else {
        sensor_lost[sensor_id] = RETRY_COUNT;
    }

    int j = 0;

    if (all) {
        int shift;
        int info_shift;

        shift = BAR_BOTTOM / count;

        switch (count) {
            case    1:
                info_shift = 14;
                shift = 33;
                break;
            case    2:
                info_shift = 5;
                break;
            case    3:
                info_shift = 2;
                shift = 10;
                break;
            default:
                info_shift = 1;
                break;
        }

        g15r_clearScreen(canvas, G15_COLOR_WHITE);
        print_vert_label(canvas, vert_label);

        int bar_top, bar_bottom;
        bar_bottom = BAR_BOTTOM;
        for (j = 0; j < count; j++) {
            bar_top = (j * shift) + 1 + j;
            bar_bottom = ((j + 1)*shift) + j;

            snprintf(tmpstr, MAX_LINES, format_main, j + 1, probes[j].cur);
            g15r_renderString(canvas, (unsigned char*) tmpstr, 0, G15_TEXT_MED, 1, bar_top + info_shift);
            drawBar_both(canvas, bar_top, bar_bottom, probes[j].cur + 1, tot_max, tot_max - probes[j].cur, tot_max);
        }
        drawLine_both(canvas, 1, bar_bottom);
    }

    if ((!all) || (info_cycle == screen_type)) {
        char extension[16];
        tmpstr[0] = '\0';
        for (j = 0; j < count; j++) {
            snprintf(extension, sizeof(extension), format_bottom, j + 1, probes[j].cur);
            if (j) {
                append_text(tmpstr, MAX_LINES, "| ");
            }
            append_text(tmpstr, MAX_LINES, extension);
        }
        g15r_renderString(canvas, (unsigned char*) tmpstr, 0, G15_TEXT_SMALL, 80 - (strlen(tmpstr)*4) / 2, INFO_ROW);
    }
}

void draw_g15_stats_info_screen(g15canvas *canvas, char *tmpstr, int all, int screen_type) {
    static g15_stats_info probes[NUM_PROBES];
    int count;
    switch (screen_type) {
        case SCREEN_TEMP:
            count = get_sensors(probes, SCREEN_TEMP, sensor_type_temp, sensor_lost_temp, sensor_temp_id);
            draw_g15_stats_info_screen_logic(canvas, tmpstr, all, screen_type, probes, 
                    count, temp_tot_max, sensor_lost_temp, sensor_temp_id, "TEMP", "t-%d %3.f\xb0", "temp%d %1.f\xb0 ");
            break;
        case SCREEN_FAN:
            count = get_sensors(probes, SCREEN_FAN, sensor_type_fan, sensor_lost_fan, sensor_fan_id);
            if (count > 0) {
                int j;
                int nonzero_seen = 0;
                for (j = 0; j < count; j++) {
                    if (probes[j].cur > 0) {
                        nonzero_seen = 1;
                        break;
                    }
                }
                if (!sensor_fan_forced && !nonzero_seen) {
                    count = 0;
                }
            }

            if (count <= 0) {
                if (all) {
                    g15r_clearScreen(canvas, G15_COLOR_WHITE);
                    g15r_renderString(canvas, (unsigned char*)"FAN missing", 0, G15_TEXT_LARGE, 80 - (11 * 9) / 2, 6);
                    g15r_renderString(canvas, (unsigned char*)"check sensor id", 0, G15_TEXT_MED, 80 - (15 * 6) / 2, 24);
                }
                if ((!all) || (info_cycle == SCREEN_FAN)) {
                    snprintf(tmpstr, MAX_LINES, "Fan sensor missing");
                    g15r_renderString(canvas, (unsigned char*)tmpstr, 0, G15_TEXT_SMALL, 80 - (strlen(tmpstr) * 4) / 2, INFO_ROW);
                }
                break;
            }
            draw_g15_stats_info_screen_logic(canvas, tmpstr, all, screen_type, probes, 
                    count, fan_tot_max, sensor_lost_fan, sensor_fan_id, "RPM", "F-%d%5.f", "Fan%d %1.f ");
            break;
        default:
            break;
    }
}

void calc_info_cycle(void) {
    int info_pause = get_info_pause();

    info_cycle = cycle;
    info_cycle_timer++;

    if (!submode) {
        int target_screen;

        target_screen = (int) (info_cycle_timer / info_pause);
        switch (target_screen) {
            case SCREEN_SUMMARY:
                info_cycle = SCREEN_SUMMARY;
                break;
            case SCREEN_CPU:
                info_cycle = SCREEN_CPU;
                break;
            case SCREEN_CPU2:
                info_cycle = SCREEN_CPU2;
                break;
            case SCREEN_FREQ_AGG:
                if (have_freq) {
                    info_cycle = SCREEN_FREQ_AGG;
                    break;
                }
                info_cycle_timer += info_pause;
            case SCREEN_MEM:
                info_cycle = SCREEN_MEM;
                break;
            case SCREEN_SWAP:
                info_cycle = SCREEN_SWAP;
                break;
            case SCREEN_NET:
                if (have_nic) {
                    info_cycle = SCREEN_NET;
                    break;
                }
                info_cycle_timer += info_pause;
            case SCREEN_BAT:
                if (have_bat) {
                    info_cycle = SCREEN_BAT;
                    break;
                }
                info_cycle_timer += info_pause;
            case SCREEN_TEMP:
                if (have_temp) {
                    info_cycle = SCREEN_TEMP;
                    break;
                }
                info_cycle_timer += info_pause;
            case SCREEN_FAN:
                if (have_fan) {
                    info_cycle = SCREEN_FAN;
                    break;
                }
                info_cycle_timer += info_pause;
            case SCREEN_NET2:
                if (have_nic) {
                    info_cycle = SCREEN_NET2;
                    break;
                }
                info_cycle_timer += info_pause;
                break;
            default:
                info_cycle_timer = 0;
                info_cycle = SCREEN_SUMMARY;
                break;
        }

        if (target_screen >= SCREEN_NET2 && info_cycle == SCREEN_NET2 && !have_nic) {
            info_cycle_timer = 0;
            info_cycle = SCREEN_SUMMARY;
        }
    }
}

void print_info_label(g15canvas *canvas, char *tmpstr) {
    switch (info_cycle) {
        case SCREEN_SUMMARY :
            print_time_info(canvas, tmpstr);
            break;
        case SCREEN_CPU :
        case SCREEN_CPU2:
            print_sys_load_info(canvas, tmpstr);
            break;
        case SCREEN_FREQ_AGG:
            break;
        case SCREEN_MEM :
            print_mem_info(canvas, tmpstr);
            break;
        case SCREEN_SWAP    :
            print_swap_info(canvas, tmpstr);
            break;
        case SCREEN_NET :
            print_net_peak_info(canvas, tmpstr);
            break;
        case SCREEN_BAT :
            if (cycle != SCREEN_BAT) {
                draw_bat_screen(canvas, tmpstr, 0);
            }
            break;
        case SCREEN_TEMP    :
            if (cycle != SCREEN_TEMP) {
                draw_g15_stats_info_screen(canvas, tmpstr, 0, SCREEN_TEMP);
            }
            break;
        case SCREEN_FAN    :
            if (cycle != SCREEN_FAN) {
                draw_g15_stats_info_screen(canvas, tmpstr, 0, SCREEN_FAN);
            }
            break;
        case SCREEN_NET2    :
            print_net_current_info(canvas, tmpstr);
            break;
    }
}

void draw_screen_overlay(g15canvas *canvas, char *tmpstr) {
    int overlay_left = 40;
    int number_x = 140;
    int y1 = 0;
    int y2 = 10;
    char number_text[16];

    if (!show_screen_overlay || overlay_ticks <= 0 || overlay_screen < 0) {
        return;
    }

    snprintf(number_text, sizeof(number_text), "#%d", overlay_screen_number(overlay_screen));

    g15r_pixelBox(canvas, overlay_left, y1, G15_LCD_WIDTH - 1, y2, G15_COLOR_WHITE, 1, 1);
    snprintf(tmpstr, MAX_LINES, "%s", screen_name(overlay_screen));
    g15r_renderString(canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, overlay_left, 1);
    g15r_renderString(canvas, (unsigned char*)number_text, 0, G15_TEXT_MED, number_x, 1);
    overlay_ticks--;
}

void draw_mode_overlay(g15canvas *canvas, char *tmpstr) {
    int overlay_left = 56;
    int y1 = 0;
    int y2 = 12;
    int x;

    if (mode_overlay_ticks <= 0 || mode_overlay_text[0] == '\0') {
        return;
    }

    snprintf(tmpstr, MAX_LINES, "%s", mode_overlay_text);
    x = G15_LCD_WIDTH - ((int) strlen(tmpstr) * 6) - 1;
    if (x < overlay_left) {
        x = overlay_left;
    }

    g15r_pixelBox(canvas, overlay_left, y1, G15_LCD_WIDTH - 1, y2, G15_COLOR_WHITE, 1, 1);
    g15r_renderString(canvas, (unsigned char*)tmpstr, 0, G15_TEXT_MED, x, 1);

    mode_overlay_ticks--;
    if (mode_overlay_ticks <= 0) {
        mode_overlay_text[0] = '\0';
    }
}

void keyboard_watch(void) {
    unsigned int keystate;
    int change   = 0;
    int mode_changed = 0;
    int submode_changed = 0;

    while(1) {
        int recv_len;

        keystate = 0;
        recv_len = recv(g15screen_fd, &keystate, 4, 0);
        if (recv_len != 4) {
            usleep(100 * 900);
            continue;
        }

        if(keystate & G15_KEY_L1) {
        }
        else if(keystate & G15_KEY_L2) {
            cycle = find_next_visible_screen(cycle, -1);
            change = CHANGE_DOWN;
        }
        else if(keystate & G15_KEY_L3) {
            cycle = find_next_visible_screen(cycle, 1);
            change = CHANGE_UP;
        }
        else if(keystate & G15_KEY_L4) {
            mode[cycle]++;
            change = CHANGE_MODE;
        }
        else if(keystate & G15_KEY_L5) {
            submode++;
            change = CHANGE_SUBMODE;
        }

        if (change) {
            switch (change) {
                case CHANGE_UP :
                    info_cycle_timer = cycle * PAUSE;
                    break;
                case CHANGE_DOWN :
                    info_cycle_timer = cycle * PAUSE;
                    break;
                case CHANGE_MODE :
                    switch (cycle) {
                        case SCREEN_NET:
                            if (have_nic) {
                                net_scale_absolute ^= 1;
                            } else {
                                change = 0;
                            }
                            break;
                        case SCREEN_TEMP:
                            sensor_temp_id = get_next(sensor_temp_id, sensor_lost_temp);
                            if (sensor_temp_id == SENSOR_ERROR) {
                                sensor_temp_id = 0;
                            }
                            break;
                        case SCREEN_FAN:
                            sensor_fan_id = get_next(sensor_fan_id, sensor_lost_fan);
                            if (sensor_fan_id == SENSOR_ERROR) {
                                sensor_fan_id = 0;
                            }
                            break;
                        case    SCREEN_SWAP:
                        case    SCREEN_MEM:
                        case    SCREEN_BAT:
                            change = 0;
                            break;
                    }
                    if (debug_enabled) {
                        fprintf(stderr,
                                "[g15stats] mode changed: screen=%d (%s) mode=%d\n",
                                cycle,
                                screen_name(cycle),
                                mode[cycle]);
                    }
                    mode_changed = 1;
                    break;
                case CHANGE_SUBMODE:
                    if (debug_enabled) {
                        fprintf(stderr,
                                "[g15stats] submode changed: submode=%d\n",
                                submode);
                    }
                    submode_changed = 1;
                    break;
            }
            if (cycle > MAX_SCREENS) {
                //Wrap around the apps
                cycle = 0;
            }

            if (mode[cycle] > MAX_MODE) {
                mode[cycle] = 0;
            }

            if (submode > MAX_SUB_MODE) {
                submode = 0;
            }

            if (mode_changed) {
                queue_mode_overlay(mode_description(cycle, mode[cycle]));
                mode_changed = 0;
            }
            if (submode_changed) {
                queue_mode_overlay(submode_description(submode));
                submode_changed = 0;
            }

            if ((change == CHANGE_UP || change == CHANGE_DOWN) && show_screen_overlay) {
                overlay_screen = cycle;
                overlay_ticks = 1;
            }

            if (change) {
                pthread_cond_broadcast(&wake_now);
            }
            change = 0;
        }
        usleep(100 * 900);
    }

    return;
}

void network_watch(void *iface) {
  char *interface = (char*)iface;
  static unsigned long previous_in;
  static unsigned long previous_out;
  int i = 0;
  glibtop_netload netload;
  int mac=0;

  glibtop_get_netload(&netload,interface);
  for(i=0;i<8;i++)
    mac+=netload.hwaddress[i];
  if(!mac && debug_enabled) {
    fprintf(stderr,
            "[g15stats] network interface %s has no MAC in netload; continuing with traffic counters\n",
            interface);
  }

    while (1) {
        int j;
        int max_in = 0;
        int max_out = 0;

        if (previous_in + previous_out) {
            net_cur_in = netload.bytes_in - previous_in;
            net_cur_out = netload.bytes_out - previous_out;
            net_hist[i][0] = net_cur_in;
            net_hist[i][1] = net_cur_out;

            if (net_scale_absolute==1) {
                net_max_in = maxi(net_max_in, net_cur_in);
                net_max_out = maxi(net_max_out, net_cur_out);
            } else {
                /* Try ti auto-resize the net graph */
                /* check for max value */
                for (j = 0; j < MAX_NET_HIST; j++) {
                    max_in = maxi(max_in, net_hist[j][0]);
                    max_out = maxi(max_out, net_hist[j][1]);
                }

                /* Assign new values */
                net_max_in = max_in;
                net_max_out = max_out;
            }
            net_rr_index = i;
            i++;
            if (i > MAX_NET_HIST) i = 0;
        }
        previous_in = netload.bytes_in;
        previous_out = netload.bytes_out;
        sleep(1);
        glibtop_get_netload(&netload, interface);
    }
}

int auto_discover_nic(unsigned char *interface, size_t interface_size) {
    DIR *dir;
    struct dirent *entry;
    glibtop_netload netload;
    char fallback_name[128] = {0};
    int have_fallback = 0;

    if (interface == NULL || interface_size == 0) {
        return 0;
    }

    dir = opendir("/sys/class/net");
    if (dir == NULL) {
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        int i;
        int mac = 0;

        if (entry->d_name[0] == '.') {
            continue;
        }
        if (strcmp(entry->d_name, "lo") == 0) {
            continue;
        }

        if (!have_fallback) {
            size_t n = strlen(entry->d_name);
            if (n > (sizeof(fallback_name) - 1)) {
                n = sizeof(fallback_name) - 1;
            }
            memcpy(fallback_name, entry->d_name, n);
            fallback_name[n] = '\0';
            have_fallback = 1;
        }

        glibtop_get_netload(&netload, entry->d_name);
        for (i = 0; i < 8; i++) {
            mac += netload.hwaddress[i];
        }
        if (!mac) {
            continue;
        }

        snprintf((char *)interface, interface_size, "%s", entry->d_name);
        closedir(dir);
        return 1;
    }

    if (have_fallback) {
        snprintf((char *)interface, interface_size, "%s", fallback_name);
        closedir(dir);
        return 1;
    }

    closedir(dir);
    return 0;
}

/* wait for a max of <seconds> seconds.. if condition &wake_now is received leave immediately */
void g15stats_wait(int seconds) {
    pthread_mutex_t dummy_mutex;
    pthread_mutexattr_t   mta;
    struct timespec timeout;
      /* Create a dummy mutex which doesn't unlock for sure while waiting. */
    pthread_mutexattr_init(&mta);

    pthread_mutex_init(&dummy_mutex, &mta);
    pthread_mutex_lock(&dummy_mutex);

    if (clock_gettime(CLOCK_REALTIME, &timeout) != 0) {
        perror("clock_gettime");
    }

    timeout.tv_sec += seconds;

    pthread_cond_timedwait(&wake_now, &dummy_mutex, &timeout);
    pthread_mutex_unlock(&dummy_mutex);
    pthread_mutex_destroy(&dummy_mutex);
    pthread_mutexattr_destroy(&mta);
}

int main(int argc, const char *argv[]){

    g15canvas *canvas;
    pthread_t keys_thread;
    pthread_t net_thread;
    
    int i;
    int go_daemon=0;
    unsigned char interface[128] = {0};
    char output_file_path[512] = {0};
    FILE *output_file = NULL;
    int use_screen_output = 1;
    static char tmpstr[MAX_LINES];
    int unicore = 0;
    int forced_screen = -1;
    int forced_mode = -1;
    
    load_config_file(&go_daemon,
                     &unicore,
                     interface,
                     &have_nic,
                     output_file_path,
                     sizeof(output_file_path));

    init_cpu_vendor_flags();

    init_battery_sensor();

    for (i = 1; i < argc; i++) {
        if (argv[i] == NULL) {
            continue;
        }

        if(0==strncmp(argv[i],"-d",2)||0==strncmp(argv[i],"--daemon",8)) {
            go_daemon=1;
        }
        if(0==strncmp(argv[i],"-u",2)||0==strncmp(argv[i],"--unicore",9)) {
            unicore=1;
        }
        if(0==strncmp(argv[i],"-nsa",4)||0==strncmp(argv[i],"--net-scale-absolute",20)) {
            net_scale_absolute=1;
        }

        if(0==strncmp(argv[i],"-df",3)||0==strncmp(argv[i],"--disable-freq",14)) {
            have_freq=0;
        }

        if(0==strncmp(argv[i],"-rc",3)||0==strncmp(argv[i],"--info-rotate",13)) {
            submode = 0;
        }

        if(0==strncmp(argv[i],"-vc",3)||0==strncmp(argv[i],"--variable-cpu",14)) {
            variable_cpu = 1;
        }
        if(0==strncmp(argv[i],"-D",2)||0==strncmp(argv[i],"--debug",7)) {
            debug_enabled = 1;
        }
        if(0==strncmp(argv[i],"--bar-background",16)) {
            bar_chart_background = 1;
        }
        if(0==strncmp(argv[i],"--cpu2-bar-height",17)) {
          if((i + 1) < argc) {
            i++;
            cpu2_bar_height = atoi(argv[i]);
            if (cpu2_bar_height < 1) {
                cpu2_bar_height = 1;
            }
            if (cpu2_bar_height > (BAR_BOTTOM + 1)) {
                cpu2_bar_height = BAR_BOTTOM + 1;
            }
          }
        }
        if(0==strncmp(argv[i],"--no-screen-overlay",19)) {
            show_screen_overlay = 0;
            overlay_ticks = 0;
        }

        if(0==strncmp(argv[i],"-h",2)||0==strncmp(argv[i],"--help",6)) {
            printf("%s %s - (c) 2008-2010 Mike Lampard, Piotr Czarnecki; 2026 Torstein Eide\n",PACKAGE_NAME,VERSION);
            printf("Usage: %s [Options]\n", PACKAGE_NAME);
            printf("Options:\n");
            printf("--daemon (-d) run in background\n");
            printf("--unicore (-u) display unicore graphs only on the CPU screen\n");
            printf("--help (-h) this help text.\n");
            printf("--interface [interface] (-i) monitor network interface [interface] ie -i eth0\n");
            printf("--temperature [id] (-t) monitor temperatures sensor [id] ie -t 1\n"
                    "\t[id] should point to sysfs path /sys/class/hwmon/hwmon[id]/device/temp1_input\n");
            printf("--global-temp [id] (-gt) force to show temperature [id] in place of the maximal one\n"
                    "\ton the Summary Screen ie -gt 1\n"
                    "\t[id] should point to sysfs path /sys/class/hwmon/hwmon../device/temp[id]_input\n");
            printf("--fan [id] (-f) monitor fans sensor [id] ie -f 1\n"
                    "\t[id] should point to sysfs path /sys/class/hwmon/hwmon[id]/device/fan1_input\n");
            printf("--net-scale-absolute (-nsa) scale net graphs against maximum speed seen.\n"
                    "\tDefault is to scale fullsize, similar to apps like gkrellm.\n");
            printf("--info-rotate (-ir) enable the bottom info bar content rotate.\n");
            printf("--variable-cpu (-vc) the cpu cores will be calculated every time (for systems with the cpu hotplug).\n");
            printf("--debug (-D) enable debug logs to stderr.\n");
            printf("--bar-background enable bar chart background boxes (default: off).\n");
            printf("--cpu2-bar-height [pixels] set target CPU LOAD2 bar height (1-%d).\n", BAR_BOTTOM + 1);
            printf("--no-screen-overlay disable one-refresh top-right overlay after L2/L3 screen change.\n");
            printf("--refresh [seconds] (-r) set the refresh interval to [seconds] The seconds must be between 1 and 300. ie -r 15\n");
            printf("--disable-freq (-df) disable monitoring CPUs frequencies.\n\n");
            printf("--output-file [path] (-o) write rendered LCD frames to [path] instead of sending to g15daemon\n");
            printf("Config file: %s (CLI options override file values).\n", get_config_file_path());
            return 0;
        }
        if(0==strncmp(argv[i],"-i",2)||0==strncmp(argv[i],"--interface",11)) {
          if((i + 1) < argc) {
            have_nic=1;
            i++;
            strncpy((char*)interface,argv[i],127);
            interface[127] = '\0';
          }
        }
        if(0==strncmp(argv[i],"-t",2)||0==strncmp(argv[i],"--temperature",13)) {
          if((i + 1) < argc) {
            i++;
            sensor_temp_id = atoi(argv[i]);
            if (sensor_temp_id >= MAX_SENSOR) {
                sensor_temp_id = 0;
            }
            sensor_temp_forced = 1;
          }
        }

        if(0==strncmp(argv[i],"-gt",3)||0==strncmp(argv[i],"--global-temp",13)) {
          if((i + 1) < argc) {
            i++;
            sensor_temp_main = atoi(argv[i]);
          }
        }

        if(0==strncmp(argv[i],"-f",2)||0==strncmp(argv[i],"--fan",5)) {
          if((i + 1) < argc) {
            i++;
            sensor_fan_id = atoi(argv[i]);
            if (sensor_fan_id >= MAX_SENSOR) {
                sensor_fan_id = 0;
            }
            sensor_fan_forced = 1;
          }
        }

        if(0==strncmp(argv[i],"-r",2)||0==strncmp(argv[i],"--refresh",9)) {
          if((i + 1) < argc) {
            i++;
            wait_seconds = atoi(argv[i]);
            if ((wait_seconds < 1) || (wait_seconds > MAX_INTERVAL)) {
                wait_seconds = 1;
            }
          }
        }
        if(0==strncmp(argv[i],"-o",2)||0==strncmp(argv[i],"--output-file",13)) {
          if((i + 1) < argc) {
            i++;
            strncpy(output_file_path, argv[i], sizeof(output_file_path)-1);
            output_file_path[sizeof(output_file_path)-1] = '\0';
          }
        }
    }        

    if (output_file_path[0] != '\0') {
        output_file = fopen(output_file_path, "ab");
        if (output_file == NULL) {
            fprintf(stderr, "Sorry, cant open output file: %s\n", output_file_path);
            return -1;
        }
        use_screen_output = 0;
    } else {
        if((g15screen_fd = new_g15_screen(G15_G15RBUF))<0){
            printf("Sorry, cant connect to the G15daemon\n");
            return -1;
        }
    }

    canvas = (g15canvas *) malloc (sizeof (g15canvas));
    if(go_daemon==1) 
        daemonise(0,0);

    if(canvas != NULL)
        g15r_initCanvas(canvas);
    else {
        if (output_file != NULL) {
            fclose(output_file);
        }
        return -1;
    }

    glibtop_init();

    if (!have_nic) {
        if (auto_discover_nic(interface, sizeof(interface))) {
            have_nic = 1;
            if (debug_enabled) {
                fprintf(stderr, "[g15stats] auto-selected network interface: %s\n", interface);
            }
        }
    }

    if (use_screen_output == 1) {
        pthread_create(&keys_thread,NULL,(void*)keyboard_watch,NULL);
    }
  
    if(have_nic==1)
      pthread_create(&net_thread,NULL,(void*)network_watch,&interface);

    forced_screen = get_forced_screen();
    forced_mode = get_forced_mode();

    for (i=0;i<MAX_SENSOR;i++) {
        sensor_lost_fan[i] = 1;
        sensor_lost_temp[i] = 1;
    }

    auto_select_sensor(SCREEN_TEMP);
    auto_select_sensor(SCREEN_FAN);

    int cycle_old   = cycle;
    if (debug_enabled) {
        fprintf(stderr,
                "[g15stats] current screen: %d (%s), mode=%d, submode=%d\n",
                cycle,
                screen_name(cycle),
                mode[cycle],
                submode);
    }
    while(1) {
        if (forced_screen >= SCREEN_SUMMARY) {
            cycle = forced_screen;
        }
        if (forced_mode >= 0) {
            mode[cycle] = forced_mode;
        }

        if (cycle != SCREEN_CPU2) {
            cpu2_debug_logged = 0;
        }

        calc_info_cycle();
        switch (cycle) {
            case SCREEN_SUMMARY:
            case SCREEN_CPU:
            case SCREEN_CPU2:
            case SCREEN_FREQ_AGG:
                draw_cpu_screen_multicore(canvas, tmpstr, unicore);
                break;
            case SCREEN_MEM:
                draw_mem_screen(canvas, tmpstr);
                break;
            case SCREEN_SWAP:
                draw_swap_screen(canvas, tmpstr);
                break;
            case SCREEN_NET:
            case SCREEN_NET2:
            case SCREEN_BAT:
            case SCREEN_TEMP:
            case SCREEN_FAN:
                if ((cycle_old != cycle) && ((!cycle_old) ^ (cycle_old < cycle))) {
                    switch (cycle) {
                        case SCREEN_FAN:
                            if (have_fan) {
                                draw_g15_stats_info_screen(canvas, tmpstr, 1, SCREEN_FAN);
                                if (have_fan) {
                                    break;
                                }
                            }
                            cycle--;
                            info_cycle  = cycle;
                        case SCREEN_TEMP:
                            if (have_temp) {
                                draw_g15_stats_info_screen(canvas, tmpstr, 1, SCREEN_TEMP);
                                if (have_temp) {
                                    break;
                                }
                            }
                            cycle--;
                            info_cycle  = cycle;
                        case SCREEN_BAT:
                            if (have_bat) {
                                draw_bat_screen(canvas, tmpstr, 1);
                                if (have_bat) {
                                    break;
                                }
                            }
                            cycle--;
                            info_cycle  = cycle;
                        case SCREEN_NET:
                        case SCREEN_NET2:
                            if (have_nic) {
                                draw_net_screen(canvas, tmpstr, (char*) interface);
                                if (have_nic) {
                                    break;
                                }
                            }
                            cycle--;
                            info_cycle  = cycle;
                        case SCREEN_SWAP:
                            draw_swap_screen(canvas, tmpstr);
                            break;
                    }
                } else {
                    switch (cycle) {
                        case SCREEN_NET:
                        case SCREEN_NET2:
                            if (have_nic) {
                                draw_net_screen(canvas, tmpstr, (char*) interface);
                                break;
                            }
                            cycle++;
                            info_cycle  = cycle;
                        case SCREEN_BAT:
                            if (have_bat) {
                                draw_bat_screen(canvas, tmpstr, 1);
                                if (have_bat) {
                                    break;
                                }
                            }
                            cycle++;
                            info_cycle  = cycle;
                        case SCREEN_TEMP:
                            if (have_temp) {
                                draw_g15_stats_info_screen(canvas, tmpstr, 1, SCREEN_TEMP);
                                if (have_temp) {
                                    break;
                                }
                            }
                            cycle++;
                            info_cycle  = cycle;
                        case SCREEN_FAN:
                            if (have_fan) {
                                draw_g15_stats_info_screen(canvas, tmpstr, 1, SCREEN_FAN);
                                if (have_fan) {
                                    break;
                                }
                            }
                            cycle++;
                            info_cycle  = cycle;
                    }
                }
                if (cycle <= MAX_SCREENS) {
                    break;
                }
            default:
                draw_cpu_screen_multicore(canvas, tmpstr, unicore);
                cycle = 0;
                info_cycle  = cycle;
                break;
        }
        if (debug_enabled && cycle_old != cycle) {
            fprintf(stderr,
                    "[g15stats] current screen: %d (%s), mode=%d, submode=%d\n",
                    cycle,
                    screen_name(cycle),
                    mode[cycle],
                    submode);
        }
        cycle_old   = cycle;
        print_info_label(canvas, tmpstr);
        draw_screen_overlay(canvas, tmpstr);
        draw_mode_overlay(canvas, tmpstr);

        canvas->mode_xor = 0;

        if (use_screen_output == 1) {
            g15_send(g15screen_fd,(char *)canvas->buffer,G15_BUFFER_LEN);
        } else {
            fwrite((char *)canvas->buffer, 1, G15_BUFFER_LEN, output_file);
            fflush(output_file);
        }
        g15stats_wait(wait_seconds);
    }
    glibtop_close();

    if (use_screen_output == 1) {
        close(g15screen_fd);
    } else {
        fclose(output_file);
    }
    free(canvas);
    return 0;  
}
