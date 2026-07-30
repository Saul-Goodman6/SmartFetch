#include "os_detect.h"

#ifdef SF_OS_WINDOWS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <sysinfoapi.h>
#include "sysinfo.h"

static void get_os_name_win(char *out, size_t size) {
    strncpy(out, "Windows", size - 1);
    out[size - 1] = '\0';

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char productName[128] = "";
        DWORD bufSize = sizeof(productName);
        if (RegQueryValueExA(hKey, "ProductName", NULL, NULL, (LPBYTE)productName, &bufSize) == ERROR_SUCCESS) {
            strncpy(out, productName, size - 1);
            out[size - 1] = '\0';
        }
        RegCloseKey(hKey);
    }
}

static void get_kernel_win(char *out, size_t size) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buildNumber[32] = "";
        DWORD bufSize = sizeof(buildNumber);
        if (RegQueryValueExA(hKey, "CurrentBuildNumber", NULL, NULL, (LPBYTE)buildNumber, &bufSize) == ERROR_SUCCESS) {
            snprintf(out, size, "NT %s", buildNumber);
            RegCloseKey(hKey);
            return;
        }
        RegCloseKey(hKey);
    }
    strncpy(out, "Windows NT", size - 1);
    out[size - 1] = '\0';
}

static void get_cpu_name_win(char *out, size_t size) {
    strncpy(out, "N/A", size);
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char cpuName[128] = "";
        DWORD bufSize = sizeof(cpuName);
        if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)cpuName, &bufSize) == ERROR_SUCCESS) {
            char *p = cpuName;
            while (*p == ' ') p++;
            strncpy(out, p, size - 1);
            out[size - 1] = '\0';
        }
        RegCloseKey(hKey);
    }
}

static void get_ram_info_win(char *out, size_t size) {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        double total_gib = (double)statex.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        double avail_gib = (double)statex.ullAvailPhys / (1024.0 * 1024.0 * 1024.0);
        double used_gib = total_gib - avail_gib;
        double percent = (double)statex.dwMemoryLoad;

        const char *color = "\033[1;32m";
        if (percent >= 80.0) color = "\033[1;31m";
        else if (percent >= 50.0) color = "\033[1;33m";

        snprintf(out, size, "%.1fGi / %.1fGi (%s%.0f%%\033[0m)", used_gib, total_gib, color, percent);
    } else {
        strncpy(out, "N/A", size);
    }
}

static void get_storage_info_win(char *out, size_t size) {
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceExA("C:\\", &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        double total_gb = (double)totalBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
        double free_gb = (double)totalFreeBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
        double used_gb = total_gb - free_gb;

        snprintf(out, size, "C: (NTFS) %.1fG/%.1fG", used_gb, total_gb);
    } else {
        strncpy(out, "N/A", size);
    }
}

static void get_screen_info_win(char *out, size_t size) {
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    if (width > 0 && height > 0) {
        snprintf(out, size, "%dx%d", width, height);
    } else {
        strncpy(out, "N/A", size);
    }
}

static void get_gpu_info_win(char *out, size_t size) {
    strncpy(out, "N/A", size);
    HKEY hKeyEnum;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}", 0, KEY_READ, &hKeyEnum) == ERROR_SUCCESS) {
        char subKeyName[256];
        DWORD index = 0;
        DWORD nameLen = sizeof(subKeyName);

        while (RegEnumKeyExA(hKeyEnum, index, subKeyName, &nameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            index++;
            nameLen = sizeof(subKeyName);
            if (strcmp(subKeyName, "Properties") == 0) continue;

            HKEY hKeySub;
            if (RegOpenKeyExA(hKeyEnum, subKeyName, 0, KEY_READ, &hKeySub) == ERROR_SUCCESS) {
                char adapterString[128] = "";
                DWORD bufSize = sizeof(adapterString);
                if (RegQueryValueExA(hKeySub, "DriverDesc", NULL, NULL, (LPBYTE)adapterString, &bufSize) == ERROR_SUCCESS) {
                    strncpy(out, adapterString, size - 1);
                    out[size - 1] = '\0';
                    RegCloseKey(hKeySub);
                    break;
                }
                RegCloseKey(hKeySub);
            }
        }
        RegCloseKey(hKeyEnum);
    }
}

static void get_shell_info_win(char *out, size_t size) {
    const char *comspec = getenv("ComSpec");
    if (comspec && comspec[0] != '\0') {
        const char *slash = strrchr(comspec, '\\');
        const char *name = slash ? slash + 1 : comspec;
        strncpy(out, name, size - 1);
        out[size - 1] = '\0';
    } else {
        strncpy(out, "cmd.exe", size - 1);
        out[size - 1] = '\0';
    }
}

static void get_os_uptime_win(char *out, size_t size) {
    ULONGLONG ms = GetTickCount64();
    long total_minutes = (long)(ms / (1000 * 60));
    long days = total_minutes / (60 * 24);
    long hours = (total_minutes / 60) % 24;
    long minutes = total_minutes % 60;

    char buf[64] = "";
    char part[32];
    int wrote = 0;

    if (days > 0) {
        snprintf(part, sizeof(part), "%ld day%s", days, days == 1 ? "" : "s");
        strcat(buf, part);
        wrote = 1;
    }
    if (hours > 0) {
        if (wrote) strcat(buf, ", ");
        snprintf(part, sizeof(part), "%ld hour%s", hours, hours == 1 ? "" : "s");
        strcat(buf, part);
        wrote = 1;
    }
    if (minutes > 0 || !wrote) {
        if (wrote) strcat(buf, ", ");
        snprintf(part, sizeof(part), "%ld minute%s", minutes, minutes == 1 ? "" : "s");
        strcat(buf, part);
    }

    strncpy(out, buf, size - 1);
    out[size - 1] = '\0';
}

void collect_system_data(SystemData *data) {
    get_os_name_win(data->os_name, sizeof(data->os_name));
    get_kernel_win(data->kernel, sizeof(data->kernel));
    get_cpu_name_win(data->cpu_name, sizeof(data->cpu_name));
    strncpy(data->cpu_temp, "N/A", sizeof(data->cpu_temp));
    get_ram_info_win(data->ram_total, sizeof(data->ram_total));
    strncpy(data->ram_type, "N/A", sizeof(data->ram_type));
    get_storage_info_win(data->storage_info, sizeof(data->storage_info));
    get_screen_info_win(data->screen_info, sizeof(data->screen_info));
    get_gpu_info_win(data->gpu_type, sizeof(data->gpu_type));
    get_shell_info_win(data->shell_info, sizeof(data->shell_info));
    strncpy(data->flatpak_count, "N/A", sizeof(data->flatpak_count));
    strncpy(data->os_age, "N/A", sizeof(data->os_age));
    get_os_uptime_win(data->os_uptime, sizeof(data->os_uptime));
}

#endif