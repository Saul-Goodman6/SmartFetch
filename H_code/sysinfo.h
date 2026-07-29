#ifndef SYSINFO_H
#define SYSINFO_H

#define SF_LINE_COUNT 11
#define SF_LINE_WIDTH 512
#define SF_ASCII_WIDTH 35

typedef struct {
    char os_name[64];
    char kernel[64];
    char cpu_name[128];
    char cpu_temp[32];
    char ram_total[32];
    char ram_type[32];
    char storage_info[512];
    char screen_info[256];
    char os_age[64];
    char os_uptime[64];
} SystemData;

void collect_system_data(SystemData *data);

void render_ui(const SystemData *data);

#endif