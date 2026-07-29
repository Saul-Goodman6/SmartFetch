#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysinfo.h"

int main(int argc, char *argv[]) {

    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_help();
            return 0;
        }
        if (strcmp(argv[1], "--update") == 0 || strcmp(argv[1], "-u") == 0) {
            check_for_update();
            return 0;
        }

        printf("Unknown option: %s\n\n", argv[1]);
        print_help();
        return 1;
    }

    SystemData sys;

    collect_system_data(&sys);

    render_ui(&sys);

    return 0;
}