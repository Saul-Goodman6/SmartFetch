#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "sysinfo.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

static void get_cmd(const char *cmd, char *output, size_t size) {
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fgets(output, size, fp) != NULL) {
            output[strcspn(output, "\n")] = 0;
        } else {
            strncpy(output, "N/A", size);
        }
        pclose(fp);
    } else {
        strncpy(output, "N/A", size);
    }
    if (output[0] == '\0') {
        strncpy(output, "N/A", size);
    }
}

static void get_exe_dir(char *out, size_t size) {
    ssize_t len = readlink("/proc/self/exe", out, size - 1);
    if (len != -1) {
        out[len] = '\0';
        char *slash = strrchr(out, '/');
        if (slash) {
            *(slash + 1) = '\0';
        } else {
            out[0] = '\0';
        }
    } else {
        out[0] = '\0';
    }
}

static int visible_len(const char *s) {
    int len = 0;
    while (*s) {
        if (*s == '\033' && *(s + 1) == '[') {
            s += 2;
            while (*s && *s != 'm') s++;
            if (*s == 'm') s++;
        } else {
            len++;
            s++;
        }
    }
    return len;
}

static FILE *open_ascii_file(const char *exe_dir, const char *ascii_name) {
    char path[PATH_MAX];

    snprintf(path, sizeof(path), "/usr/share/smartfetch/Ascii_art/%s", ascii_name);
    FILE *fp = fopen(path, "r");
    if (fp) return fp;

    snprintf(path, sizeof(path), "%sAscii_art/%s", exe_dir, ascii_name);
    fp = fopen(path, "r");
    if (fp) return fp;

    snprintf(path, sizeof(path), "Ascii_art/%s", ascii_name);
    return fopen(path, "r");
}

void collect_system_data(SystemData *data) {
    get_cmd("grep -E '^PRETTY_NAME=' /etc/os-release | cut -d= -f2 | tr -d '\"'", data->os_name, sizeof(data->os_name));
    get_cmd("uname -r", data->kernel, sizeof(data->kernel));
    get_cmd("lscpu | grep 'Model name' | awk -F: '{print $2}' | xargs", data->cpu_name, sizeof(data->cpu_name));

    get_cmd("sensors 2>/dev/null | grep -iE 'package id|core 0|cpu' | head -n1 | awk '{print $4}' | tr -d '+'", data->cpu_temp, sizeof(data->cpu_temp));
    if (strcmp(data->cpu_temp, "N/A") == 0 || strlen(data->cpu_temp) == 0) {
        get_cmd("cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null | awk '{printf \"%.1f°C\", $1/1000}'", data->cpu_temp, sizeof(data->cpu_temp));
    }

    char ram_total_val[16], ram_used[16];
    get_cmd("free -h | grep Mem | awk '{print $2}' | awk -F'(' '{print $1}' | xargs", ram_total_val, sizeof(ram_total_val));
    get_cmd("free -h | grep Mem | awk '{print $3}'", ram_used, sizeof(ram_used));
    snprintf(data->ram_total, sizeof(data->ram_total), "%s / %s", ram_used, ram_total_val);

    get_cmd("dmidecode --type memory 2>/dev/null | grep 'Type:' | grep -v 'Unknown' | head -n1 | awk '{print $2}'", data->ram_type, sizeof(data->ram_type));
    get_cmd("lsblk -d -o NAME,SIZE,MODEL | grep -v 'loop' | tr '\n' ' '", data->storage_info, sizeof(data->storage_info));

    get_cmd("xrandr 2>/dev/null | grep '*' | awk '{print $1}' | head -n1", data->screen_info, sizeof(data->screen_info));
    if (strcmp(data->screen_info, "N/A") == 0) {
        get_cmd("wlr-randr 2>/dev/null | grep -oE '[0-9]+x[0-9]+' | head -n1", data->screen_info, sizeof(data->screen_info));
    }

    get_cmd("stat -c %w / 2>/dev/null | cut -d' ' -f1", data->os_age, sizeof(data->os_age));
    get_cmd("uptime -p | sed 's/up //'", data->os_uptime, sizeof(data->os_uptime));
}

void render_ui(const SystemData *data) {
    char exe_dir[PATH_MAX];
    get_exe_dir(exe_dir, sizeof(exe_dir));

    const char *ascii_name = "default_ascii.txt";
    if (strstr(data->os_name, "Arch") != NULL || strstr(data->os_name, "arch") != NULL) {
        ascii_name = "arch_ascii.txt";
    } else if (strstr(data->os_name, "Debian") != NULL || strstr(data->os_name, "debian") != NULL) {
        ascii_name = "debian_ascii.txt";
    } else if (strstr(data->os_name, "Fedora") != NULL || strstr(data->os_name, "fedora") != NULL) {
        ascii_name = "fedora_ascii.txt";
    }

    FILE *ascii_fp = open_ascii_file(exe_dir, ascii_name);

    char ascii_line[128];
    int line_index = 0;

    printf("\n");

    char hostname[HOST_NAME_MAX];
    char *username = getenv("USER");
    if (username == NULL) username = getlogin();
    if (username == NULL) username = "user";

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        snprintf(hostname, sizeof(hostname), "pc");
    }

    char info_lines[SF_LINE_COUNT][SF_LINE_WIDTH];
    snprintf(info_lines[0], SF_LINE_WIDTH, "\033[1;36m%s\033[0m@\033[1;36m%s\033[0m", username, hostname);
    
    
    int user_host_len = strlen(username) + strlen(hostname) + 1;
    char separator[128] = "";
    for (int i = 0; i < user_host_len && i < 127; i++) {
        separator[i] = '-';
    }
    separator[user_host_len] = '\0';

    snprintf(info_lines[1], SF_LINE_WIDTH, "%s", separator);
    snprintf(info_lines[2], SF_LINE_WIDTH, "\033[1;32mOS       :\033[0m %s", data->os_name);
    snprintf(info_lines[3], SF_LINE_WIDTH, "\033[1;32mKernel   :\033[0m %s", data->kernel);
    snprintf(info_lines[4], SF_LINE_WIDTH, "\033[1;32mUptime   :\033[0m %s", data->os_uptime);
    snprintf(info_lines[5], SF_LINE_WIDTH, "\033[1;32mOS Age   :\033[0m %s", data->os_age);
    snprintf(info_lines[6], SF_LINE_WIDTH, "\033[1;32mCPU      :\033[0m %s", data->cpu_name);
    snprintf(info_lines[7], SF_LINE_WIDTH, "\033[1;32mCPU Temp :\033[0m %s", data->cpu_temp);
    snprintf(info_lines[8], SF_LINE_WIDTH, "\033[1;32mRAM      :\033[0m %s (%s)", data->ram_total, data->ram_type);
    snprintf(info_lines[9], SF_LINE_WIDTH, "\033[1;32mStorage  :\033[0m %s", data->storage_info);
    snprintf(info_lines[10], SF_LINE_WIDTH, "\033[1;32mDisplay  :\033[0m %s", data->screen_info);

    while (1) {
        char *got_ascii = NULL;
        if (ascii_fp) {
            got_ascii = fgets(ascii_line, sizeof(ascii_line), ascii_fp);
            if (got_ascii) {
                ascii_line[strcspn(ascii_line, "\n")] = 0;
            }
        }

        int has_info = line_index < SF_LINE_COUNT;

        if (!got_ascii && !has_info) {
            break;
        }

        if (got_ascii) {
            int pad = SF_ASCII_WIDTH - visible_len(ascii_line);
            printf("%s", ascii_line);
            for (int i = 0; i < pad; i++) putchar(' ');
            putchar(' ');
        } else {
            for (int i = 0; i < SF_ASCII_WIDTH + 1; i++) putchar(' ');
        }

        if (has_info) {
            printf("%s", info_lines[line_index]);
        }

        printf("\n");
        line_index++;
    }

    if (ascii_fp) fclose(ascii_fp);
    printf("\n");
}