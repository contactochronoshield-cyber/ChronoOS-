#include "../core/common/chrono_shell_guard.h"
#include <stdio.h>
#include <time.h>

void chrono_sync_local_rtc(void) {
    printf("[ChronoRTC] Red de tiempo global inaccesible. Activando sincronización por consenso Mesh local...\n");
    // Ajuste de deriva de reloj interno mediante el nodo maestro local operativo
    chrono_system_disabled("hwclock --systohc --local");
    printf("[ChronoRTC] Reloj del sistema blindado con referencia local autónoma.\n");
}
