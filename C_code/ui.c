#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
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

void render_ui(const SystemData *data) {
    char exe_dir[PATH_MAX];
    get_exe_dir(exe_dir, sizeof(exe_dir));

    const char *ascii_name = "default_ascii.txt";

    if (strstr(data->os_name, "CachyOS") != NULL || strstr(data->os_name, "cachyos") != NULL) {
        ascii_name = "cashyos_ascii.txt";
    } else if (strstr(data->os_name, "EndeavourOS") != NULL || strstr(data->os_name, "endeavouros") != NULL) {
        ascii_name = "endeavouros_ascii.txt";
    } else if (strstr(data->os_name, "Arch") != NULL || strstr(data->os_name, "arch") != NULL) {
        ascii_name = "arch_ascii.txt";
    } else if (strstr(data->os_name, "Fedora") != NULL || strstr(data->os_name, "fedora") != NULL) {
        ascii_name = "fedora_ascii.txt";
    } else if (strstr(data->os_name, "Linux Mint") != NULL || strstr(data->os_name, "linuxmint") != NULL) {
        ascii_name = "linuxmint_ascii.txt";
    } else if (strstr(data->os_name, "Ubuntu") != NULL || strstr(data->os_name, "ubuntu") != NULL) {
        ascii_name = "ubuntu_ascii.txt";
    } else if (strstr(data->os_name, "Debian GNU/Linux") != NULL || strstr(data->os_name, "debian") != NULL) {
        ascii_name = "debian_ascii.txt";
    } else if (strstr(data->os_name, "Windows") != NULL || strstr(data->os_name, "windows") != NULL) {
        ascii_name = "windows_ascii.txt";
    }

    FILE *ascii_fp = open_ascii_file(exe_dir, ascii_name);

    char ascii_line[128];
    int line_index = 0;

    printf("\n");

    char hostname[HOST_NAME_MAX];
    char *username = NULL;

    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_name) {
        username = pw->pw_name;
    } else {
        username = getenv("USER");
        if (username == NULL) username = getlogin();
        if (username == NULL) username = "user";
    }

    if (gethostname(hostname, sizeof(hostname)) != 0 || strlen(hostname) == 0) {
        snprintf(hostname, sizeof(hostname), "smartfetch");
    }

    char info_lines[SF_LINE_COUNT][SF_LINE_WIDTH];
    memset(info_lines, 0, sizeof(info_lines));
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
    snprintf(info_lines[11], SF_LINE_WIDTH, "\033[1;32mGPU      :\033[0m %s", data->gpu_type);
    snprintf(info_lines[12], SF_LINE_WIDTH, "\033[1;32mShell    :\033[0m %s", data->shell_info);
    snprintf(info_lines[13], SF_LINE_WIDTH, "\033[1;32mFlatpak  :\033[0m %s", data->flatpak_count);

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

    
    print_color_palette();
    printf("\n");
}

void print_help(void) {
    printf("SmartFetch (sfetch) - v%s\n", SF_VERSION);
    printf("A fast tool to display system information with distro ASCII logo.\n\n");
    printf("Usage:\n");
    printf("  sfetch                  Display system information\n");
    printf("  sfetch -h, --help       Display this help message\n");
    printf("  sfetch -u, --update     Check for new updates on GitHub\n");
}

void check_for_update(void) {
    char remote_hash[64];
    char cmd[256];

    snprintf(cmd, sizeof(cmd),
        "git ls-remote %s refs/heads/main 2>/dev/null | awk '{print $1}'",
        SF_REPO_URL);
    get_cmd(cmd, remote_hash, sizeof(remote_hash));

    if (strcmp(remote_hash, "N/A") == 0 || strlen(remote_hash) == 0) {
        printf("Could not check for updates.\n");
        printf("Make sure you are connected to the internet and git is installed.\n");
        return;
    }

    if (strcmp(SF_VERSION, "unknown") == 0) {
        printf("Cannot determine current version (installed without git info).\n");
        printf("Latest version on GitHub: %.7s\n", remote_hash);
        printf("To update run: git pull origin main && make && sudo make install\n");
        return;
    }

    if (strncmp(remote_hash, SF_VERSION, strlen(SF_VERSION)) == 0) {
        printf("You are on the latest version (%s).\n", SF_VERSION);
    } else {
        printf("A new update is available!\n");
        printf("Current version : %s\n", SF_VERSION);
        printf("Latest version  : %.7s\n", remote_hash);
        printf("\nTo update, go to the project folder and run:\n");
        printf("  git pull origin main\n");
        printf("  make && sudo make install\n");
    }
}