#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>

void mount_virtual_filesystems() {
    printf("[*] ChronoInit: Montando sistemas de archivos virtuales...\n");
    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
}

int main() {
    if (getpid() != 1) {
        fprintf(stderr, "[!] Este programa debe correr como PID 1.\n");
        return 1;
    }

    printf("[+] ChronoOS Init System Iniciado.\n");
    mount_virtual_filesystems();

    if (fork() == 0) {
        execl("/bin/sh", "sh", "/etc/chrono/services.sh", NULL);
        exit(0);
    }

    while (1) {
        wait(NULL);
    }
    return 0;
}
