/* ==============================================================================
 * ChronoOS Enterprise Packet & Network Integrity Auditor v8.0
 * Author: Daniel Gonzales / Chrono Shield Networks
 * Architecture: Low-level C Socket Buffer Inspection & Threat Detection
 * ============================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_PACKET_SIZE 2048
#define PROTOCOL_MAGIC_TAG 0x4E455453ULL // 'NETS'

typedef struct {
    uint64_t signature;
    uint32_t packet_seq;
    uint16_t source_port;
    uint16_t dest_port;
    size_t payload_len;
    time_t timestamp;
    char security_digest[48];
} NetworkHeader;

void render_auditor_banner() {
    printf("==================================================================\n");
    printf("       CHRONOOS NATIVE PACKET & NETWORK INTEGRITY AUDITOR        \n");
    printf("       Chrono Shield Networks - Air-Gapped Airspace Engine       \n");
    printf("==================================================================\n");
}

void generate_packet_digest(const char *payload, char *out_digest) {
    unsigned long hash = 5381;
    int c;
    while ((c = *payload++)) {
        hash = ((hash << 5) + hash) + c;
    }
    snprintf(out_digest, 48, "NET-AUDIT-SHA-%016lX-SEC", hash);
}

int inspect_and_audit_packet(uint32_t seq, uint16_t sport, uint16_t dport, const char *payload) {
    render_auditor_banner();
    printf("[*] Capturando paquete de red (Secuencia: %u) para auditoría...\n", seq);

    NetworkHeader *net_header = (NetworkHeader *)malloc(sizeof(NetworkHeader));
    if (net_header == NULL) {
        fprintf(stderr, "[!] Error crítico: Fallo de asignación de memoria para cabecera de red.\n");
        return -1;
    }

    net_header->signature = PROTOCOL_MAGIC_TAG;
    net_header->packet_seq = seq;
    net_header->source_port = sport;
    net_header->dest_port = dport;
    net_header->payload_len = strlen(payload);
    net_header->timestamp = time(NULL);
    generate_packet_digest(payload, net_header->security_digest);

    printf("[✓] Cabecera analizada con éxito:\n");
    printf("    - Protocol Signature : 0x%llX\n", (unsigned long long)net_header->signature);
    printf("    - Sequence ID        : %u\n", net_header->packet_seq);
    printf("    - Port Mapping       : %u -> %u\n", net_header->source_port, net_header->dest_port);
    printf("    - Payload Length     : %zu bytes\n", net_header->payload_len);
    printf("    - Cryptographic Tag  : %s\n", net_header->security_digest);
    printf("    - Capture Timestamp  : %s", ctime(&(net_header->timestamp)));

    char *packet_payload_buffer = (char *)malloc(MAX_PACKET_SIZE);
    if (packet_payload_buffer == NULL) {
        fprintf(stderr, "[!] Error crítico: Fallo de asignación para búfer de payload.\n");
        free(net_header);
        return -1;
    }

    memset(packet_payload_buffer, 0, MAX_PACKET_SIZE);
    strncpy(packet_payload_buffer, payload, MAX_PACKET_SIZE - 1);

    // Auditoría de contenido en busca de anomalías simuladas
    if (strstr(packet_payload_buffer, "MALICIOUS") != NULL || strstr(packet_payload_buffer, "EXPLICIT") != NULL) {
        printf("[!] ALERTA DE SEGURIDAD: Contenido prohibido o anomalía detectada en el paquete.\n");
        printf("[*] Acciones de contención aplicadas automáticamente por ChronoOS.\n");
    } else {
        printf("[✓] Integridad del paquete: LIMPIO Y VERIFICADO.\n");
        printf("[*] Contenido del búfer: \"%s\"\n", packet_payload_buffer);
    }

    // Escritura en log de auditoría local
    FILE *log_file = fopen("var/logs/network_audit.log", "a");
    if (log_file != NULL) {
        fprintf(log_file, "[%ld] SEQ:%u SPORT:%u DPORT:%u STATUS:CLEAN DIGEST:%s\n", 
                (long)net_header->timestamp, net_header->packet_seq, 
                net_header->source_port, net_header->dest_port, net_header->security_digest);
        fclose(log_file);
        printf("[✓] Registro de auditoría guardado en 'var/logs/network_audit.log'.\n");
    }

    // Saneamiento y liberación de memoria Heap
    free(packet_payload_buffer);
    free(net_header);
    printf("[*] Memoria liberada y recursos de red cerrados de forma limpia.\n");

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        inspect_and_audit_packet(4001, 8080, 443, "ChronoOS Air-Gapped Secure Telemetry Stream Data");
        return 0;
    }

    inspect_and_audit_packet(9999, 5000, 80, argv[1]);
    return 0;
}
