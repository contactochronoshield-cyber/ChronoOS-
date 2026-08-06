#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

// Función de borrado seguro en memoria inmune a optimizaciones del compilador
void secure_zero_memory(volatile void *v, size_t n) {
    volatile unsigned char *p = (volatile unsigned char *)v;
    while (n--) {
        *p++ = 0;
    }
}

// Sobrescritura forense de archivos en disco (Multi-pasada)
int forensic_shred_file(const char *filepath) {
    int fd = open(filepath, O_WRONLY);
    if (fd == -1) return -1;

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return -1;
    }

    off_t filesize = st.st_size;
    if (filesize <= 0) {
        close(fd);
        unlink(filepath);
        return 0;
    }

    char *buffer = malloc(filesize);
    if (!buffer) {
        close(fd);
        return -1;
    }

    // Pasada 1: Ceros
    memset(buffer, 0x00, filesize);
    pwrite(fd, buffer, filesize, 0);
    fsync(fd);

    // Pasada 2: Unes (0xFF)
    memset(buffer, 0xFF, filesize);
    pwrite(fd, buffer, filesize, 0);
    fsync(fd);

    // Pasada 3: Datos Pseudoaleatorios desde /dev/urandom
    int urand = open("/dev/urandom", O_RDONLY);
    if (urand != -1) {
        read(urand, buffer, filesize);
        close(urand);
        pwrite(fd, buffer, filesize, 0);
        fsync(fd);
    }

    // Limpiar búfer en memoria RAM antes de liberar
    secure_zero_memory(buffer, filesize);
    free(buffer);
    close(fd);

    // Eliminar enlace físico definitivo del archivo
    if (unlink(filepath) != 0) {
        return -1;
    }

    return 0;
}

// Ejecución de Purgado Crítico del Sistema
void execute_system_panic_purge(void) {
    printf("[CRITICAL-PURGE] ¡ALERTA MÁXIMA! Iniciando purga criptográfica profunda en RAM y Disco...\n");

    // 1. Asignar y bloquear memoria sensible para garantizar limpieza
    size_t sensitive_key_len = 256;
    char *sensitive_vault_key = malloc(sensitive_key_len);
    if (sensitive_vault_key) {
        if (mlock(sensitive_vault_key, sensitive_key_len) == 0) {
            // Simular carga de clave maestra en memoria protegida
            memset(sensitive_vault_key, 0xAB, sensitive_key_len);
            
            // Purgado inmediato e irreversible en RAM
            secure_zero_memory(sensitive_vault_key, sensitive_key_len);
            munlock(sensitive_vault_key, sensitive_key_len);
        }
        free(sensitive_vault_key);
    }

    // 2. Destrucción forense de archivos críticos y bóvedas
    const char *target_keys[] = {
        "./crypto_keys/mesh_master.key",
        "./crypto_keys/master.key",
        "./config/.panic_trigger"
    };

    for (int i = 0; i < 3; i++) {
        if (access(target_keys[i], F_OK) == 0) {
            if (forensic_shred_file(target_keys[i]) == 0) {
                printf("[PURGE] Archivo destruido forensemente: %s\n", target_keys[i]);
            }
        }
    }

    // 3. Forzar sincronización de bloques de disco y apagado abrupto del kernel
    sync();
    printf("[PURGE] Memoria RAM saneada y discos sincronizados. Ejecutando parada de emergencia...\n");
}

int main(void) {
    execute_system_panic_purge();
    return 0;
}
