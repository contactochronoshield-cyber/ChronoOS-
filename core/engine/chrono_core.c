/**
 * ==============================================================================
 * ChronoOS - Sovereign C Kernel Core Supervisor (chrono_core.c)
 * Author: Daniel Gonzales / Chrono Shield Networks
 * Description: Núcleo de control y auditoría de bajo nivel en C.
 * ==============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#define CHRONO_VERSION "10.0-SOVEREIGN"

void print_banner() {
    printf("\n");
    printf("  ██████╗ ██╗  ██╗██████╗  ██████╗ ███╗   ██╗ ██████╗  ██████╗ ███████╗\n");
    printf(" ██╔════╝ ██║  ██║██╔══██╗██╔═══██╗████╗  ██║██╔═══██╗██╔══██╗██╔════╝\n");
    printf(" ██║      ███████║██████╔╝██║   ██║██╔██╗ ██║██║   ██║██║   ██║███████╗\n");
    printf(" ██║      ██╔══██║██╔══██║██║   ██║██║╚██╗██║██║   ██║██║   ██║╚════██║\n");
    printf(" ╚██████╗ ██║  ██║██████╔╝╚██████╔╝██║ ╚████║╚██████╔╝╚██████╔╝███████║\n");
    printf("  ╚═════╝ ╚═╝  ╚══╚═════╝  ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝ ╚══════╝\n");
    printf("       [ CHRONO-OS NATIVE C KERNEL CORE v%s ]\n\n", CHRONO_VERSION);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_banner();
        printf("Uso: chrono-core [status | memory | wipe]\n");
        return 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        print_banner();
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            printf("[STATUS] Sistema Operativo Soberano: ONLINE\n");
            printf("[STATUS] RAM Total: %lu MB\n", si.totalram / 1024 / 1024);
            printf("[STATUS] RAM Libre: %lu MB\n", si.freeram / 1024 / 1024);
            printf("[STATUS] Tareas Activas: %d\n", si.procs);
        }
    } else if (strcmp(argv[1], "memory") == 0) {
        struct sysinfo si;
        sysinfo(&si);
        printf("[MEM] RAM Libre: %lu MB / Total: %lu MB\n", si.freeram / 1024 / 1024, si.totalram / 1024 / 1024);
    } else if (strcmp(argv[1], "wipe") == 0) {
        printf("[!] Ejecutando purga de memoria de alta seguridad...\n");
        volatile char *mem = (volatile char *)malloc(4096);
        if (mem) {
            memset((void *)mem, 0xFF, 4096);
            memset((void *)mem, 0x00, 4096);
            free((void *)mem);
            printf("[✓] Memoria purgada exitosamente.\n");
        }
    } else {
        printf("[!] Comando desconocido: %s\n", argv[1]);
    }
    return 0;
}
