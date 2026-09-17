/**
 * ==============================================================================
 * ChronoOS - True Sovereign PID 1 Initialization Engine (chrono_init.c)
 * Author: Daniel Gonzales / Chrono Shield Networks
 * Description: Proceso de arranque principal PID 1. Monta sistemas de archivos 
 *              virtuales y gestiona la consola del sistema operativo.
 * ==============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>

void mount_virtual_filesystems() {
    printf("[CHRONO-INIT] Montando sistemas de archivos virtuales del kernel...\n");
    
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        perror("[!] Error montando /proc");
    }
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0) {
        perror("[!] Error montando /sys");
    }
    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) != 0) {
        perror("[!] Error montando /dev");
    }
}

int main() {
    if (getpid() != 1) {
        printf("[!] Error crítico: chrono_init debe ejecutarse estrictamente como PID 1.\n");
        return 1;
    }

    printf("\n");
    printf("  ============================================================\n");
    printf("   CHRONO-OS KERNEL BOOT // PID 1 INITIALIZATION ENGAGED        \n");
    printf("  ============================================================\n");

    mount_virtual_filesystems();

    printf("[✓] Entorno de archivos base asegurado. Lanzando interfaz segura...\n");

    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = { "/bin/sh", NULL };
        execve("/bin/sh", argv, NULL);
        perror("[!] Error ejecutando /bin/sh");
        exit(1);
    } else if (pid > 0) {
        int status;
        while (1) {
            pid_t terminated = wait(&status);
            if (terminated == pid) {
                printf("[!] Consola principal finalizada. Reiniciando servicio...\n");
                pid = fork();
                if (pid == 0) {
                    char *argv[] = { "/bin/sh", NULL };
                    execve("/bin/sh", argv, NULL);
                    exit(1);
                }
            }
        }
    } else {
        perror("[!] Error crítico en fork de PID 1");
    }

    return 0;
}
