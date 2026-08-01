#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void logo() {
    printf("\n");
    printf("  ██████╗ ██╗  ██╗██████╗  ██████╗ ███╗   ██╗ ██████╗  ██████╗ ███████╗\n");
    printf(" ██╔════╝ ██║  ██║██╔══██╗██╔═══██╗████╗  ██║██╔═══██╗██╔═══██╗██╔════╝\n");
    printf(" ██║      ███████║██████╔╝██║   ██║██╔██╗ ██║██║   ██║██║   ██║███████╗\n");
    printf(" ██║      ██╔══██║██╔══██╗██║   ██║██║╚██╗██║██║   ██║██║   ██║╚════██║\n");
    printf(" ╚██████╗ ██║  ██║██████╔╝╚██████╔╝██║ ╚████║╚██████╔╝╚██████╔╝███████║\n");
    printf("  ╚═════╝ ╚oxide╚══╚═════╝  ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝ ╚══════╝\n");
    printf("       [ SOVEREIGN TACTICAL OS // AIR-GAPPED KERNEL v8.0 ]\n\n");
}

int main() {
    logo();
    printf("[*] ChronoInit: Iniciando secuencia de arranque soberano...\n");

    // Montar sistemas de archivos virtuales esenciales
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        perror("[!] Error montando /proc");
    }
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0) {
        perror("[!] Error montando /sys");
    }
    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) != 0) {
        perror("[!] Error montando /dev");
    }

    printf("[✓] Sistemas de archivos virtuales montados correctamente.\n");
    printf("[*] Ejecutando autodiagnóstico de la Entidad Viva...\n");
    
    // Ejecutar auditoría de la entidad local
    system("./core/entity/evolution.sh pulse");

    printf("[*] Abriendo consola de control táctico ChronoOS...\n");
    
    // Iniciar shell interactiva de control
    char *argv[] = { "/bin/sh", NULL };
    execv("/bin/sh", argv);

    perror("[!] Error crítico: No se pudo lanzar el shell del sistema.");
    return 1;
}
