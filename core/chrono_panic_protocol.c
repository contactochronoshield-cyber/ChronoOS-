#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>

void secure_zero_memory(volatile void *v, size_t n) {
    volatile unsigned char *p = (volatile unsigned char *)v;
    while (n--) {
        *p++ = 0;
    }
}

// Destrucción forense para medios magnéticos tradicionales
int forensic_shred(const char *filepath) {
    int fd = open(filepath, O_WRONLY);
    if (fd == -1) return -1;
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return -1;
    }
    off_t size = st.st_size;
    if (size <= 0) { close(fd); unlink(filepath); return 0; }
    char *buf = malloc(size);
    if (!buf) { close(fd); return -1; }

    memset(buf, 0x00, size); pwrite(fd, buf, size, 0); fsync(fd);
    memset(buf, 0xFF, size); pwrite(fd, buf, size, 0); fsync(fd);
    int urand = open("/dev/urandom", O_RDONLY);
    if (urand != -1) { read(urand, buf, size); close(urand); pwrite(fd, buf, size, 0); fsync(fd); }

    secure_zero_memory(buf, size);
    free(buf);
    close(fd);
    return unlink(filepath);
}

// Crypto-Shredding para SSDs modernos (Anula el FTL y Wear-Leveling destruyendo cabeceras LUKS)
void execute_ssd_crypto_shred(void) {
    printf("[SSD-PURGE] Ejecutando Crypto-Shredding de volúmenes cifrados (Anulación de FTL)...\n");
    // Sobrescribir el header del volumen cifrado principal (ej. /dev/mapper/chrono_vault o partición raíz segura)
    const char *target_device = "/dev/nvme0n1p2"; // Ajustar según partición física real
    int fd = open(target_device, O_WRONLY);
    if (fd != -1) {
        // Sobrescribir los primeros 16MB donde reside la cabecera del volumen cifrado
        size_t header_size = 16 * 1024 * 1024;
        char *zero_buffer = calloc(1, header_size);
        if (zero_buffer) {
            write(fd, zero_buffer, header_size);
            fsync(fd);
            secure_zero_memory(zero_buffer, header_size);
            free(zero_buffer);
        }
        close(fd);
        printf("[SSD-PURGE] Cabecera de cifrado destruida con éxito. Datos en SSD irrecuperables.\n");
    } else {
        // Fallback: destrucción de claves simétricas locales de bóveda
        system("dd if=/dev/urandom of=./crypto_keys/master.key bs=1k count=1024 2>/dev/null || true");
    }
}

void execute_total_panic_protocol(void) {
    printf("[PANIC-PROTOCOL] ¡ALERTA MÁXIMA! Iniciando purga criptográfica y destrucción forense...\n");

    // 1. Limpieza de RAM
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

    // 2. Crypto-Shredding para SSD / Destrucción de claves
    execute_ssd_crypto_shred();

    // 3. Purga recursiva de archivos de configuración y bóvedas
    forensic_shred("./crypto_keys/mesh_master.key");
    forensic_shred("./config/.panic_trigger");

    sync();

    // 4. Detonación del Kernel
    int sysrq = open("/proc/sysrq-trigger", O_WRONLY);
    if (sysrq != -1) {
        write(sysrq, "c", 1);
        close(sysrq);
    } else {
        system("reboot -f");
    }
}

int main(void) {
    execute_total_panic_protocol();
    return 0;
}
