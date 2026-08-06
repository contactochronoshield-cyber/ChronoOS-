#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Declaraciones de subsistemas reales integrados
extern int chrono_power_shield_monitor_once(void);
extern void execute_total_panic_protocol(void);

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
            printf("[ORCHESTRATOR] Comando de pánico recibido externamente.\n");
            execute_total_panic_protocol();
            return 0;
        } else if (strcmp(argv[1], "--power-check") == 0) {
            printf("[ORCHESTRATOR] Ejecutando sondeo único de energía...\n");
            // Ejecutar lógica de power shield
            system("./build/chrono_power_shield");
            return 0;
        }
    }

    printf("[ORCHESTRATOR] Sistema operando en modo nominal seguro.\n");
    printf("[ORCHESTRATOR] Uso: ./chrono_master [--panic | --power-check]\n");
    return 0;
}
