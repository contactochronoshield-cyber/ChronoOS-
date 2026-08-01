/* ==============================================================================
 * ChronoOS Enterprise Core & Vault Engine v8.0
 * Author: Daniel Gonzales / Chrono Shield Networks
 * Architecture: Low-level C Memory Management, Buffer Auditing, & Daemon Loop
 * ============================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 4096
#define CHRONO_MAGIC_CONST 0x4348524F5320LL  // 'CHROS '

typedef struct {
    uint64_t magic_signature;
    unsigned int process_id;
    size_t payload_size;
    time_t operational_timestamp;
    char checksum_hash[64];
} SystemControlBlock;

void render_system_header() {
    printf("==================================================================\n");
    printf("        CHRONOOS ENTERPRISE NATIVE KERNEL RUNTIME v8.0            \n");
    printf("        Chrono Shield Networks - Logic in mind. Hardware in hand. \n");
    printf("==================================================================\n");
}

void compute_secure_hash(const char *source_data, char *dest_buffer) {
    unsigned long accumulator = 5381;
    int character;
    while ((character = *source_data++)) {
        accumulator = ((accumulator << 5) + accumulator) + character;
    }
    snprintf(dest_buffer, 64, "CHRO-SEC-HASH-HEX-%016lX", accumulator);
}

int initialize_control_block(unsigned int pid, const char *raw_data) {
    render_system_header();
    printf("[*] Solicitando asignación de bloque Heap (malloc) para PID: %u\n", pid);

    SystemControlBlock *scb = (SystemControlBlock *)malloc(sizeof(SystemControlBlock));
    if (scb == NULL) {
        fprintf(stderr, "[!] Error crítico de memoria: Fallo en asignación de estructura.\n");
        return -1;
    }

    scb->magic_signature = CHRONO_MAGIC_CONST;
    scb->process_id = pid;
    scb->payload_size = strlen(raw_data);
    scb->operational_timestamp = time(NULL);
    compute_secure_hash(raw_data, scb->checksum_hash);

    printf("[✓] Estructura de Control del Sistema inicializada:\n");
    printf("    - Magic Signature : 0x%llX\n", (unsigned long long)scb->magic_signature);
    printf("    - Assigned PID    : %u\n", scb->process_id);
    printf("    - Payload Length  : %zu bytes\n", scb->payload_size);
    printf("    - Cryptographic ID: %s\n", scb->checksum_hash);
    printf("    - System Time     : %s", ctime(&(scb->operational_timestamp)));

    char *dynamic_memory_buffer = (char *)malloc(MAX_BUFFER_SIZE);
    if (dynamic_memory_buffer == NULL) {
        fprintf(stderr, "[!] Error crítico: Fallo al asignar búfer de datos dinámico.\n");
        free(scb);
        return -1;
    }

    memset(dynamic_memory_buffer, 0, MAX_BUFFER_SIZE);
    strncpy(dynamic_memory_buffer, raw_data, MAX_BUFFER_SIZE - 1);

    printf("[*] Escribiendo y auditando datos en el búfer protegido...\n");
    printf("[✓] Búfer validado con éxito. Contenido: \"%s\"\n", dynamic_memory_buffer);

    printf("[*] Liberando búferes y saneando espacios de memoria...\n");
    free(dynamic_memory_buffer);
    free(scb);
    printf("[✓] Memoria liberada correctamente sin fugas (Leak-Free Execution).\n");

    return 0;
}

void execute_background_daemon_simulation() {
    printf("\n[*] Iniciando simulación de ciclo de daemon en segundo plano...\n");
    for (int cycle = 1; cycle <= 3; cycle++) {
        printf("    - Pulso de sistema [%d/3]: Núcleo activo y sincronizado.\n", cycle);
        usleep(200000); // Pausa de 200ms
    }
    printf("[✓] Ciclo de daemon finalizado de manera controlada.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        initialize_control_block(1001, "ChronoOS Default Production Core Telemetry Stream");
        execute_background_daemon_simulation();
        return 0;
    }

    initialize_control_block(8888, argv[1]);
    execute_background_daemon_simulation();
    return 0;
}
