#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdint.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <mntent.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include "sysinfo.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* ---------- OS name (/etc/os-release) ---------- */
static void get_os_name(char *out, size_t size) {
    strncpy(out, "N/A", size);
    FILE *fp = fopen("/etc/os-release", "r");
    if (!fp) return;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *value = line + 12;
            value[strcspn(value, "\n")] = 0;
            size_t len = strlen(value);
            if (len >= 2 && value[0] == '"' && value[len - 1] == '"') {
                value[len - 1] = '\0';
                value++;
            }
            strncpy(out, value, size - 1);
            out[size - 1] = '\0';
            break;
        }
    }
    fclose(fp);
}

/* ---------- Kernel (uname syscall) ---------- */
static void get_kernel(char *out, size_t size) {
    struct utsname u;
    if (uname(&u) == 0) {
        strncpy(out, u.release, size - 1);
        out[size - 1] = '\0';
    } else {
        strncpy(out, "N/A", size);
    }
}

/* ---------- CPU model name (/proc/cpuinfo) ---------- */
static void get_cpu_name(char *out, size_t size) {
    strncpy(out, "N/A", size);
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) return;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "model name", 10) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                colon[strcspn(colon, "\n")] = 0;
                strncpy(out, colon, size - 1);
                out[size - 1] = '\0';
            }
            break;
        }
    }
    fclose(fp);
}

/* ---------- CPU temperature (/sys/class/thermal) ---------- */
static void get_cpu_temp(char *out, size_t size) {
    strncpy(out, "N/A", size);

    DIR *d = opendir("/sys/class/thermal");
    if (!d) return;

    char best_path[PATH_MAX] = "";
    char fallback_path[PATH_MAX] = "";
    struct dirent *entry;

    while ((entry = readdir(d)) != NULL) {
        if (strncmp(entry->d_name, "thermal_zone", 12) != 0) continue;

        char type_path[PATH_MAX];
        snprintf(type_path, sizeof(type_path), "/sys/class/thermal/%s/type", entry->d_name);

        char type[64] = "";
        FILE *tf = fopen(type_path, "r");
        if (tf) {
            if (fgets(type, sizeof(type), tf)) type[strcspn(type, "\n")] = 0;
            fclose(tf);
        }

        char temp_path[PATH_MAX];
        snprintf(temp_path, sizeof(temp_path), "/sys/class/thermal/%s/temp", entry->d_name);

        if (fallback_path[0] == '\0') {
            strncpy(fallback_path, temp_path, sizeof(fallback_path) - 1);
        }

        if (strcasestr(type, "x86_pkg_temp") || strcasestr(type, "cpu")) {
            strncpy(best_path, temp_path, sizeof(best_path) - 1);
            break;
        }
    }
    closedir(d);

    const char *chosen = best_path[0] ? best_path : fallback_path;
    if (chosen[0] == '\0') return;

    FILE *tf = fopen(chosen, "r");
    if (!tf) return;
    long milli = 0;
    if (fscanf(tf, "%ld", &milli) == 1) {
        snprintf(out, size, "%.1f°C", milli / 1000.0);
    }
    fclose(tf);
}

/* ---------- RAM usage (/proc/meminfo) ---------- */
static double kb_to_gib(long kb) {
    return kb / (1024.0 * 1024.0);
}

/* Pick a color code based on usage percentage: <50% green, 50-79% yellow, >=80% red */
static const char *usage_color(double percent) {
    if (percent < 50.0) return "\033[1;32m";
    if (percent < 80.0) return "\033[1;33m";
    return "\033[1;31m";
}

static void get_ram_info(char *out, size_t size) {
    strncpy(out, "N/A", size);
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return;

    long mem_total = -1, mem_available = -1;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (mem_total < 0 && strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line + 9, "%ld", &mem_total);
        } else if (mem_available < 0 && strncmp(line, "MemAvailable:", 13) == 0) {
            sscanf(line + 13, "%ld", &mem_available);
        }
        if (mem_total >= 0 && mem_available >= 0) break;
    }
    fclose(fp);

    if (mem_total < 0) return;

    if (mem_available >= 0) {
        long used_kb = mem_total - mem_available;
        double percent = (double)used_kb / (double)mem_total * 100.0;
        snprintf(out, size, "%.1fGi / %.1fGi (%s%.0f%%\033[0m)",
                 kb_to_gib(used_kb), kb_to_gib(mem_total),
                 usage_color(percent), percent);
    } else {
        snprintf(out, size, "%.1fGi total", kb_to_gib(mem_total));
    }
}

/* ---------- RAM type ---------- */
static const char *mem_type_name(uint8_t t) {
    static const char *low_types[] = {
        NULL, "Other", "Unknown", "DRAM", "EDRAM", "VRAM", "SRAM", "RAM",
        "ROM", "FLASH", "EEPROM", "FEPROM", "EPROM", "CDRAM", "3DRAM",
        "SDRAM", "SGRAM", "RDRAM", "DDR", "DDR2", "DDR2 FB-DIMM"
    };
    if (t >= 1 && t <= 20) return low_types[t];
    switch (t) {
        case 0x18: return "DDR3";
        case 0x19: return "FBD2";
        case 0x1A: return "DDR4";
        case 0x1B: return "LPDDR";
        case 0x1C: return "LPDDR2";
        case 0x1D: return "LPDDR3";
        case 0x1E: return "LPDDR4";
        case 0x20: return "HBM";
        case 0x21: return "HBM2";
        case 0x22: return "DDR5";
        case 0x23: return "LPDDR5";
        default: return NULL;
    }
}

static void get_ram_type(char *out, size_t size) {
    strncpy(out, "N/A", size);

    FILE *fp = fopen("/sys/firmware/dmi/tables/DMI", "rb");
    if (!fp) return;

    unsigned char buf[65536];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    if (n < 4) return;

    size_t offset = 0;
    const char *found = NULL;

    while (offset + 4 <= n) {
        uint8_t type = buf[offset];
        uint8_t length = buf[offset + 1];
        if (length < 4 || offset + length > n) break;

        if (type == 17 && length > 0x12) {
            const char *name = mem_type_name(buf[offset + 0x12]);
            if (name && strcmp(name, "Unknown") != 0) {
                found = name;
                break;
            }
        }
        if (type == 127) break;

        size_t str_off = offset + length;
        while (str_off + 1 < n && !(buf[str_off] == 0 && buf[str_off + 1] == 0)) {
            str_off++;
        }
        offset = str_off + 2;
    }

    if (found) {
        strncpy(out, found, size - 1);
        out[size - 1] = '\0';
    }
}

/* ---------- Storage info ---------- */
static void get_storage_info(char *out, size_t size) {
    strncpy(out, "N/A", size);

    FILE *mtab = setmntent("/proc/mounts", "r");
    if (!mtab) return;

    struct mntent entry;
    char strings_buf[4096];
    struct mntent *m;

    char device[256] = "";
    char fstype[64] = "";
    int found = 0;

    while ((m = getmntent_r(mtab, &entry, strings_buf, sizeof(strings_buf))) != NULL) {
        if (strcmp(m->mnt_dir, "/") == 0) {
            strncpy(device, m->mnt_fsname, sizeof(device) - 1);
            strncpy(fstype, m->mnt_type, sizeof(fstype) - 1);
            found = 1;
        }
    }
    endmntent(mtab);

    if (!found) return;

    struct statvfs vfs;
    if (statvfs("/", &vfs) != 0) {
        snprintf(out, size, "%s (%s)", device, fstype);
        return;
    }

    double total_gb = (double)vfs.f_blocks * vfs.f_frsize / (1024.0 * 1024.0 * 1024.0);
    double free_gb = (double)vfs.f_bfree * vfs.f_frsize / (1024.0 * 1024.0 * 1024.0);
    double used_gb = total_gb - free_gb;

    snprintf(out, size, "%s (%s) %.1fG/%.1fG", device, fstype, used_gb, total_gb);
}

/* ---------- Screen info ---------- */
static void get_screen_info(char *out, size_t size) {
    strncpy(out, "N/A", size);

    DIR *d = opendir("/sys/class/drm");
    if (!d) return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char status_path[PATH_MAX];
        snprintf(status_path, sizeof(status_path), "/sys/class/drm/%s/status", entry->d_name);

        char status[32] = "";
        FILE *sf = fopen(status_path, "r");
        if (!sf) continue;
        if (fgets(status, sizeof(status), sf)) status[strcspn(status, "\n")] = 0;
        fclose(sf);

        if (strcmp(status, "connected") != 0) continue;

        char modes_path[PATH_MAX];
        snprintf(modes_path, sizeof(modes_path), "/sys/class/drm/%s/modes", entry->d_name);

        FILE *mf = fopen(modes_path, "r");
        if (!mf) continue;

        char mode[64] = "";
        if (fgets(mode, sizeof(mode), mf)) {
            mode[strcspn(mode, "\n")] = 0;
            strncpy(out, mode, size - 1);
            out[size - 1] = '\0';
        }
        fclose(mf);

        if (out[0] != '\0' && strcmp(out, "N/A") != 0) break;
    }
    closedir(d);
}

/* ---------- GPU info ---------- */
static int pci_ids_lookup(const char *path, unsigned vendor, unsigned device,
                           char *vname, size_t vname_size, char *dname, size_t dname_size) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;

    char line[512];
    int in_vendor = 0, found_vendor = 0, found_device = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (line[0] == '\t' && line[1] == '\t') continue;

        if (line[0] != '\t') {
            unsigned v;
            if (sscanf(line, "%4x", &v) == 1) {
                if (v == vendor) {
                    in_vendor = 1;
                    found_vendor = 1;
                    char *name = line + 4;
                    while (*name == ' ' || *name == '\t') name++;
                    name[strcspn(name, "\n")] = 0;
                    strncpy(vname, name, vname_size - 1);
                    vname[vname_size - 1] = '\0';
                } else if (in_vendor) {
                    break;
                }
            }
        } else if (in_vendor) {
            unsigned dv;
            if (sscanf(line + 1, "%4x", &dv) == 1 && dv == device) {
                char *name = line + 1 + 4;
                while (*name == ' ' || *name == '\t') name++;
                name[strcspn(name, "\n")] = 0;
                strncpy(dname, name, dname_size - 1);
                dname[dname_size - 1] = '\0';
                found_device = 1;
                break;
            }
        }
    }
    fclose(fp);
    return found_vendor && found_device;
}

static void get_gpu_info(char *out, size_t size) {
    strncpy(out, "N/A", size);

    DIR *d = opendir("/sys/bus/pci/devices");
    if (!d) return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char class_path[PATH_MAX];
        snprintf(class_path, sizeof(class_path), "/sys/bus/pci/devices/%s/class", entry->d_name);

        unsigned class_code = 0;
        FILE *cf = fopen(class_path, "r");
        if (!cf) continue;
        int ok = (fscanf(cf, "0x%x", &class_code) == 1);
        fclose(cf);
        if (!ok) continue;

        if (((class_code >> 16) & 0xFF) != 0x03) continue;

        char vendor_path[PATH_MAX], device_path[PATH_MAX];
        snprintf(vendor_path, sizeof(vendor_path), "/sys/bus/pci/devices/%s/vendor", entry->d_name);
        snprintf(device_path, sizeof(device_path), "/sys/bus/pci/devices/%s/device", entry->d_name);

        unsigned vendor = 0, device = 0;
        FILE *vf = fopen(vendor_path, "r");
        if (vf) { if (fscanf(vf, "0x%x", &vendor) != 1) vendor = 0; fclose(vf); }
        FILE *df = fopen(device_path, "r");
        if (df) { if (fscanf(df, "0x%x", &device) != 1) device = 0; fclose(df); }

        char vname[128] = "", dname[128] = "";
        int resolved = 0;
        const char *ids_paths[] = {
            "/usr/share/hwdata/pci.ids",
            "/usr/share/misc/pci.ids",
            "/usr/share/pci.ids"
        };
        for (size_t i = 0; i < sizeof(ids_paths) / sizeof(ids_paths[0]) && !resolved; i++) {
            resolved = pci_ids_lookup(ids_paths[i], vendor, device, vname, sizeof(vname), dname, sizeof(dname));
        }

        if (resolved) {
            snprintf(out, size, "%s %s", vname, dname);
        } else {
            snprintf(out, size, "PCI %04x:%04x", vendor, device);
        }
        break;
    }
    closedir(d);
}

/* ---------- Shell info ---------- */
static void get_shell_info(char *out, size_t size) {
    strncpy(out, "N/A", size);
    const char *shell = getenv("SHELL");
    if (!shell || shell[0] == '\0') return;
    const char *slash = strrchr(shell, '/');
    const char *name = slash ? slash + 1 : shell;
    strncpy(out, name, size - 1);
    out[size - 1] = '\0';
}

/* ---------- Flatpak count ---------- */
static int count_dir_entries(const char *path) {
    DIR *d = opendir(path);
    if (!d) return -1;
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        count++;
    }
    closedir(d);
    return count;
}

static void get_flatpak_count(char *out, size_t size) {
    int total = 0;
    int checked_any = 0;

    int sys_count = count_dir_entries("/var/lib/flatpak/app");
    if (sys_count >= 0) { total += sys_count; checked_any = 1; }

    const char *home = getenv("HOME");
    if (home) {
        char user_path[PATH_MAX];
        snprintf(user_path, sizeof(user_path), "%s/.local/share/flatpak/app", home);
        int user_count = count_dir_entries(user_path);
        if (user_count >= 0) { total += user_count; checked_any = 1; }
    }

    if (!checked_any) {
        strncpy(out, "N/A", size);
    } else {
        snprintf(out, size, "%d", total);
    }
}

/* ---------- OS age ---------- */
static void get_os_age(char *out, size_t size) {
    strncpy(out, "N/A", size);
    struct statx stx;
    if (statx(AT_FDCWD, "/", 0, STATX_BTIME, &stx) == 0 && (stx.stx_mask & STATX_BTIME)) {
        time_t btime = stx.stx_btime.tv_sec;
        struct tm tm_info;
        localtime_r(&btime, &tm_info);
        strftime(out, size, "%Y-%m-%d", &tm_info);
    }
}

/* ---------- Uptime ---------- */
static void get_os_uptime(char *out, size_t size) {
    strncpy(out, "N/A", size);
    FILE *fp = fopen("/proc/uptime", "r");
    if (!fp) return;
    double seconds = 0;
    int ok = (fscanf(fp, "%lf", &seconds) == 1);
    fclose(fp);
    if (!ok) return;

    long total_minutes = (long)(seconds / 60);
    long days = total_minutes / (60 * 24);
    long hours = (total_minutes / 60) % 24;
    long minutes = total_minutes % 60;

    char buf[64] = "";
    char part[32];
    int wrote = 0;

    if (days > 0) {
        snprintf(part, sizeof(part), "%ld day%s", days, days == 1 ? "" : "s");
        strncat(buf, part, sizeof(buf) - strlen(buf) - 1);
        wrote = 1;
    }
    if (hours > 0) {
        if (wrote) strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
        snprintf(part, sizeof(part), "%ld hour%s", hours, hours == 1 ? "" : "s");
        strncat(buf, part, sizeof(buf) - strlen(buf) - 1);
        wrote = 1;
    }
    if (minutes > 0 || !wrote) {
        if (wrote) strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
        snprintf(part, sizeof(part), "%ld minute%s", minutes, minutes == 1 ? "" : "s");
        strncat(buf, part, sizeof(buf) - strlen(buf) - 1);
    }

    strncpy(out, buf, size - 1);
    out[size - 1] = '\0';
}

/* ---------- Progress Bar Generator ---------- */
void make_progress_bar(char *out, size_t size, double percentage, int width) {
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;

    int filled = (int)((percentage / 100.0) * width);
    char bar[128] = "";

    strcat(bar, "[");
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            strcat(bar, "█");
        } else {
            strcat(bar, "░");
        }
    }
    snprintf(out, size, "%s] %.0f%%", bar, percentage);
}

/* ---------- Color Palette Generator ---------- */
void print_color_palette(void) {
    printf("   ");
    for (int i = 0; i < 8; i++) {
        printf("\033[4%dm   \033[0m", i);
    }
    printf("\n   ");
    for (int i = 0; i < 8; i++) {
        printf("\033[10%dm   \033[0m", i);
    }
    printf("\n");
}

/* ---------- Collect Entry Point ---------- */
void collect_system_data(SystemData *data) {
    get_os_name(data->os_name, sizeof(data->os_name));
    get_kernel(data->kernel, sizeof(data->kernel));
    get_cpu_name(data->cpu_name, sizeof(data->cpu_name));
    get_cpu_temp(data->cpu_temp, sizeof(data->cpu_temp));
    get_ram_info(data->ram_total, sizeof(data->ram_total));
    get_ram_type(data->ram_type, sizeof(data->ram_type));
    get_storage_info(data->storage_info, sizeof(data->storage_info));
    get_screen_info(data->screen_info, sizeof(data->screen_info));
    get_gpu_info(data->gpu_type, sizeof(data->gpu_type));
    get_shell_info(data->shell_info, sizeof(data->shell_info));
    get_flatpak_count(data->flatpak_count, sizeof(data->flatpak_count));
    get_os_age(data->os_age, sizeof(data->os_age));
    get_os_uptime(data->os_uptime, sizeof(data->os_uptime));
}