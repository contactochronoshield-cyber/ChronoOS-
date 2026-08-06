#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>

void mount_virtual_filesystems() {
    printf("[CHRONO-KERNEL] Montando sistemas de archivos virtuales...\n");
    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    mkdir("/dev/pts", 0755);
    mount("devpts", "/dev/pts", "devpts", 0, NULL);
    mkdir("/run", 0755);
    mount("tmpfs", "/run", "tmpfs", 0, NULL);
}

void mount_persistent_storage() {
    mkdir("/mnt/data", 0755);
    if (mount("/dev/vda", "/mnt/data", "ext4", 0, NULL) == 0) {
        printf("[✓] Almacenamiento persistente montado en /mnt/data\n");
    } else {
        printf("[i] Sin disco persistente detectado (boot en RAM solamente)\n");
    }
}

void chrono_heartbeat_service() {
    // Servicio propio: corre en segundo plano, prueba de vida real del OS
    pid_t svc = fork();
    if (svc == 0) {
        while (1) {
            FILE *f = fopen("/mnt/data/heartbeat.log", "a");
            if (f) {
                time_t t = time(NULL);
                fprintf(f, "[chrono-heartbeat] PID %d activo en %ld\n", getpid(), (long)t);
                fclose(f);
            }
            sleep(10);
        }
        exit(0);
    }
}

int main() {
    if (getpid() != 1) {
        printf("[!] ERROR FATAL: chrono_init debe ser PID 1.\n");
        return 1;
    }

    printf("\n  ============================================================\n");
    printf("   CHRONO-OS SOVEREIGN KERNEL INITIALIZED // PID 1 ACTIVE      \n");
    printf("  ============================================================\n");

    mount_virtual_filesystems();
    mount_persistent_storage();
    chrono_heartbeat_service();

    printf("[✓] Entorno soberano configurado. Servicios iniciados. Consola lista...\n");

    char *envp[] = { "PATH=/usr/bin:/bin:/usr/sbin:/sbin", "HOME=/root", "TERM=linux", NULL };

    while (1) {
        pid_t pid = fork();
        if (pid == 0) {
            char *argv[] = { "/bin/sh", NULL };
            execve("/bin/sh", argv, envp);
            perror("[!] Error ejecutando consola");
            exit(1);
        } else if (pid > 0) {
            int status;
            pid_t terminated = wait(&status);
            printf("[!] Proceso (PID: %d) finalizado. Reiniciando subsistema...\n", terminated);
            sleep(1);
        } else {
            perror("[!] Error en fork de PID 1");
            sleep(2);
        }
    }
    return 0;
}
