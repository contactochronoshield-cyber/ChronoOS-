#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/sysinfo.h>

// 1. Limpieza segura en RAM inmune a optimizaciones
void secure_zero_memory(volatile void *v, size_t n) {
    volatile unsigned char *p = (volatile unsigned char *)v;
    while (n--) {
        *p++ = 0;
    }
}

// 2. Destrucción forense multi-pasada de archivos individuales
int forensic_shred(const char *filepath) {
    int fd = open(filepath, O_WRONLY);
    if (fd == -1) return -1;

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return -1;
    }

    off_t size = st.st_size;
    if (size <= 0) {
        close(fd);
        unlink(filepath);
        return 0;
    }

    char *buf = malloc(size);
    if (!buf) {
        close(fd);
        return -1;
    }

    // Pasada 1: Ceros (0x00)
    memset(buf, 0x00, size);
    pwrite(fd, buf, size, 0);
    fsync(fd);

    // Pasada 2: Unes (0xFF)
    memset(buf, 0xFF, size);
    pwrite(fd, buf, size, 0);
    fsync(fd);

    // Pasada 3: Ruido aleatorio (/dev/urandom)
    int urand = open("/dev/urandom", O_RDONLY);
    if (urand != -1) {
        read(urand, buf, size);
        close(urand);
        pwrite(fd, buf, size, 0);
        fsync(fd);
    }

    secure_zero_memory(buf, size);
    free(buf);
    close(fd);

    return unlink(filepath);
}

// 3. Recorrido recursivo para destruir bóvedas y carpetas de datos
void recursive_purge_directory(const char *base_path) {
    DIR *dir = opendir(base_path);
    if (!dir) return;

    struct dirent *entry;
    char path[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);
        
        struct stat st;
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                recursive_purge_directory(path);
                rmdir(path);
            } else {
                forensic_shred(path);
            }
        }
    }
    closedir(dir);
}

// 4. Ejecución del Protocolo de Pánico Total
void execute_total_panic_protocol(void) {
    printf("[PANIC-PROTOCOL] ¡ALERTA MÁXIMA! Ejecutando Protocolo de Pánico y Purga Criptográfica Total...\n");

    // A. Purgado de Memoria sensible en RAM
    size_t ram_lock_size = 4096;
    volatile char *sensitive_buffer = malloc(ram_lock_size);
    if (sensitive_buffer) {
        if (mlock((void *)sensitive_buffer, ram_lock_size) == 0) {
            memset((void *)sensitive_buffer, 0xAA, ram_lock_size);
            secure_zero_memory(sensitive_buffer, ram_lock_size);
            munlock((void *)sensitive_buffer, ram_lock_size);
        }
        free((void *)sensitive_buffer);
    }

    // B. Destrucción de archivos de claves distribuidos
    const char *key_targets[] = {
        "./crypto_keys/mesh_master.key",
        "./crypto_keys/master.key",
        "./config/.panic_trigger",
        "./config/.last_heartbeat"
    };
    
    for (int i = 0; i < 4; i++) {
        if (access(key_targets[i], F_OK) == 0) {
            forensic_shred(key_targets[i]);
            printf("[PANIC] Llave destruida forensemente: %s\n", key_targets[i]);
        }
    }

    // C. Destrucción total de bóvedas de datos
    printf("[PANIC] Purgando bóvedas y registros en ./vault/ y ./var/...\n");
    recursive_purge_directory("./vault");
    recursive_purge_directory("./var");

    // D. Sincronización forzada de bloques de disco
    sync();

    // E. Activación de Pánico del Kernel (SysRq Crash o Apagado Forzoso inmediato)
    // Escribir 'c' en /proc/sysrq-trigger provoca un Kernel Crash instantáneo (anti-forense definitivo)
    int sysrq = open("/proc/sysrq-trigger", O_WRONLY);
    if (sysrq != -1) {
        printf("[PANIC] Forzando Kernel Panic para bloqueo físico de la plataforma...\n");
        write(sysrq, "c", 1);
        close(sysrq);
    } else {
        // Fallback si no hay permisos de root: Reinicio forzado por hardware
        system("reboot -f");
    }
}

int main(void) {
    execute_total_panic_protocol();
    return 0;
}
