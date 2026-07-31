#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "sysinfo.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

static void get_cmd_output(char *const argv[], char *output, size_t size) {
    if (size == 0) return;
    output[0] = '\0';

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        strncpy(output, "N/A", size - 1);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        strncpy(output, "N/A", size - 1);
        return;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execvp(argv[0], argv);
        _exit(127);
    }

    close(pipefd[1]);
    FILE *fp = fdopen(pipefd[0], "r");
    if (fp) {
        if (fgets(output, size, fp) != NULL) {
            output[strcspn(output, "\n")] = 0;
        }
        fclose(fp);
    } else {
        close(pipefd[0]);
    }

    int status;
    waitpid(pid, &status, 0);

    if (output[0] == '\0') {
        strncpy(output, "N/A", size - 1);
    }
}

static void get_exe_dir(char *out, size_t size) {
    out[0] = '\0';
    if (size == 0) return;

    ssize_t len = readlink("/proc/self/exe", out, size - 1);
    if (len <= 0 || (size_t)len >= size) {
        out[0] = '\0';
        return;
    }

    out[len] = '\0';
    char *slash = strrchr(out, '/');
    if (slash) {
        *(slash + 1) = '\0';
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

typedef struct {
    const char *needle;
    const char *file;
} os_match_t;

static const char *pick_ascii_name(const char *os_name) {
    static const os_match_t os_matches[] = {
        {"CachyOS", "cashyos_ascii.txt"},
        {"cachyos", "cashyos_ascii.txt"},
        {"EndeavourOS", "endeavouros_ascii.txt"},
        {"endeavouros", "endeavouros_ascii.txt"},
        {"Arch", "arch_ascii.txt"},
        {"arch", "arch_ascii.txt"},
        {"Fedora", "fedora_ascii.txt"},
        {"fedora", "fedora_ascii.txt"},
        {"Linux Mint", "linuxmint_ascii.txt"},
        {"linuxmint", "linuxmint_ascii.txt"},
        {"Ubuntu", "ubuntu_ascii.txt"},
        {"ubuntu", "ubuntu_ascii.txt"},
        {"Debian GNU/Linux", "debian_ascii.txt"},
        {"debian", "debian_ascii.txt"},
        {"Windows", "windows_ascii.txt"},
        {"windows", "windows_ascii.txt"},
    };

    for (size_t i = 0; i < sizeof(os_matches) / sizeof(os_matches[0]); i++) {
        if (strstr(os_name, os_matches[i].needle) != NULL) {
            return os_matches[i].file;
        }
    }
    return "default_ascii.txt";
}

static void resolve_identity(const char **username_out, char *hostname, size_t hostname_size) {
    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_name) {
        *username_out = pw->pw_name;
    } else {
        const char *env_user = getenv("USER");
        *username_out = env_user ? env_user : "user";
    }

    if (gethostname(hostname, hostname_size) != 0 || strlen(hostname) == 0) {
        snprintf(hostname, hostname_size, "smartfetch");
    }
}

static void build_info_lines(char info_lines[SF_LINE_COUNT][SF_LINE_WIDTH],
                              const SystemData *data,
                              const char *username,
                              const char *hostname) {
    memset(info_lines, 0, SF_LINE_COUNT * SF_LINE_WIDTH);

    snprintf(info_lines[0], SF_LINE_WIDTH, "\033[1;36m%s\033[0m@\033[1;36m%s\033[0m", username, hostname);

    int user_host_len = strlen(username) + strlen(hostname) + 1;
    char separator[128] = "";
    for (int i = 0; i < user_host_len && i < 127; i++) {
        separator[i] = '-';
    }
    separator[user_host_len < 127 ? user_host_len : 127] = '\0';

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
}

static void print_body(FILE *ascii_fp, char info_lines[SF_LINE_COUNT][SF_LINE_WIDTH]) {
    char ascii_line[128];
    int line_index = 0;

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
}

void render_ui(const SystemData *data) {
    char exe_dir[PATH_MAX];
    get_exe_dir(exe_dir, sizeof(exe_dir));

    const char *ascii_name = pick_ascii_name(data->os_name);
    FILE *ascii_fp = open_ascii_file(exe_dir, ascii_name);

    printf("\n");

    char hostname[HOST_NAME_MAX];
    const char *username = NULL;
    resolve_identity(&username, hostname, sizeof(hostname));

    char info_lines[SF_LINE_COUNT][SF_LINE_WIDTH];
    build_info_lines(info_lines, data, username, hostname);

    print_body(ascii_fp, info_lines);

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
    char line[256];
    char remote_hash[64] = "N/A";

    char *argv[] = { "git", "ls-remote", (char *)SF_REPO_URL, "refs/heads/main", NULL };
    get_cmd_output(argv, line, sizeof(line));

    if (strcmp(line, "N/A") != 0 && line[0] != '\0') {
        char *tab = strchr(line, '\t');
        size_t len = tab ? (size_t)(tab - line) : strlen(line);
        if (len >= sizeof(remote_hash)) len = sizeof(remote_hash) - 1;
        memcpy(remote_hash, line, len);
        remote_hash[len] = '\0';
    }

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