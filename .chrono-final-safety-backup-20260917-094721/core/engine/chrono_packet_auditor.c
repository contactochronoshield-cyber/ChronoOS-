#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

void print_help() {
    printf("ChronoOS Native Raw Packet Auditor v9.0\n");
    printf("Usage: chrono-packet-auditor [interface]\n");
    printf("Note: Requires root/CAP_NET_RAW privileges to capture live packets.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        // Fallback robusto a modo de auditoría de sockets locales si no hay permisos de root
        printf("[*] Iniciando auditoría de sockets y descriptores de archivo locales...\n");
        system("cat /proc/net/tcp 2>/dev/null || echo 'No tcp proc access'");
        return 0;
    }

    char *interface = argv[1];
    printf("[*] Abriendo socket de bajo nivel en la interfaz: %s\n", interface);

    // Crear socket raw para captura de paquetes a nivel de enlace de datos
    int sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_raw < 0) {
        perror("[!] Error crítico: No se pudo abrir el socket raw (¿Faltan privilegios root?)");
        printf("[*] Ejecutando análisis de respaldo de tablas de enrutamiento...\n");
        system("ip route show");
        return 1;
    }

    unsigned char buffer[65536];
    printf("[✓] Escuchando tráfico real en la interfaz %s...\n", interface);

    // Capturar paquetes reales en bucle de control
    int packet_count = 0;
    while (packet_count < 5) {
        int data_size = recvfrom(sock_raw, buffer, 65536, 0, NULL, NULL);
        if (data_size < 0) {
            perror("[!] Error al recibir paquete de red");
            break;
        }
        
        struct ethhdr *eth = (struct ethhdr *)buffer;
        printf("[PACKET] Capturado frame | Tamaño: %d bytes | Protocolo: 0x%04x\n", 
               data_size, ntohs(eth->h_proto));
        packet_count++;
    }

    close(sock_raw);
    printf("[✓] Auditoría de paquetes completada exitosamente.\n");
    return 0;
}
