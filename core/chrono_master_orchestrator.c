#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_system_banner(void) {
    printf("==================================================\n");
    printf("  CHRONO OS - CORE ORCHESTRATOR (PRODUCTION REAL) \n");
    printf("  Standards: ISO/IEC 62443 FR6 (Availability/Resilience)\n");
    printf("==================================================\n");
}

int main(int argc, char *argv[]) {
    print_system_banner();

    if (argc > 1) {
        if (strcmp(argv[1], "--panic") == 0) {
            printf("[ORCHESTRATOR] Ejecutando binario de pánico crítico...\n");
            execl("./build/chrono_panic_protocol", "chrono_panic_protocol", NULL);
            perror("[ERROR] No se pudo ejecutar el binario de pánico");
            return 1;
        } else if (strcmp(argv[1], "--power-check") == 0) {
            printf("[ORCHESTRATOR] Ejecutando sondeo de energía...\n");
            system("./build/chrono_power_shield");
            return 0;
        }
    }

    printf("[ORCHESTRATOR] Sistema operando en modo nominal seguro.\n");
    printf("[ORCHESTRATOR] Uso: ./build/chrono_master [--panic | --power-check]\n");
    return 0;
}
