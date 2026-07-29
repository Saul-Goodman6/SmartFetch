#include <stdlib.h>
#include "sysinfo.h"

int main() {

    SystemData sys;

    collect_system_data(&sys);

    render_ui(&sys);

    return 0;
}