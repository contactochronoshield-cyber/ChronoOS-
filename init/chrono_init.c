/**
 * ==============================================================================
 * ChronoOS - Sovereign Native PID 1 Kernel Initializer (chrono_init.c)
 * Author: Daniel Gonzales / Chrono Shield Networks
 * Description: Proceso inicial absoluto del sistema operativo (PID 1). 
 *              Configura el entorno de ejecución, monta sistemas de archivos 
 *              virtuales y gestiona los daemons principales sin intermediarios.
 * ==============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>

void mount_virtual_filesystems() {
    printf("[CHRONO-KERNEL] Montando sistemas de archivos virtuales esenciales...\n");
    
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        perror("[!] Error crítico montando /proc");
    }
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0) {
        perror("[!] Error crítico montando /sys");
    }
    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) != 0) {
        perror("[!] Error crítico montando /dev");
    }
}

int main() {
    if (getpid() != 1) {
        printf("[!] ERROR FATAL: chrono_init debe ejecutarse estrictamente como PID 1.\n");
        return 1;
    }

    printf("\n");
    printf("  ============================================================\n");
    printf("   CHRONO-OS SENDER KERNEL INITIALIZED // PID 1 ACTIVE          \n");
    printf("  ============================================================\n");

    mount_virtual_filesystems();

    printf("[✓] Entorno soberano configurado. Iniciando consola del sistema...\n");

    while (1) {
        pid_t pid = fork();
        if (pid == 0) {
            char *argv[] = { "/bin/sh", NULL };
            char *envp[] = { "PATH=/usr/bin:/bin:/usr/sbin:/sbin", "HOME=/root", "TERM=linux", NULL }; execve("/bin/sh", argv, envp);
            perror("[!] Error ejecutando consola de sistema");
            exit(1);
        } else if (pid > 0) {
            int status;
            pid_t terminated = wait(&status);
            printf("[!] Proceso de consola (PID: %d) finalizado. Reiniciando subsistema...\n", terminated);
            sleep(1);
        } else {
            perror("[!] Error crítico en fork de PID 1");
            sleep(2);
        }
    }

    return 0;
}
