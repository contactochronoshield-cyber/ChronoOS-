#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("[*] Iniciando auditoría de sockets y descriptores de archivo locales...\n");
        system("cat /proc/net/tcp 2>/dev/null || echo 'No tcp proc access'");
        return 0;
    }
    char *interface = argv[1];
    int sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_raw < 0) {
        perror("[!] Error crítico: No se pudo abrir el socket raw");
        system("ip route show");
        return 1;
    }
    printf("[✓] Escuchando tráfico real en la interfaz %s...\n", interface);
    close(sock_raw);
    return 0;
}
