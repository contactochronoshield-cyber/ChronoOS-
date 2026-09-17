/**
 * ChronoOS - Community Alert (Chrono Conjunto)
 * Sistema de alerta de emergencia para conjuntos residenciales que NO
 * depende de internet ni de red celular. Los nodos (garita, administracion,
 * torres/bloques) se comunican via mesh local (WireGuard sobre la red
 * del conjunto). Si un nodo activa una alerta, se propaga a todos los
 * demas nodos conectados, sin pasar por ningun proveedor externo.
 *
 * Disenado pensando en el escenario real: sismo, corte de luz general,
 * o saturacion de red celular - momentos donde WhatsApp/apps en la nube
 * dejan de responder justo cuando mas se necesitan.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define ALERT_PORT 9001
#define MAX_PEERS 32
#define ALERT_LOG "./var/logs/community_alerts.log"

typedef struct {
    char ip[16];
    char label[64];
} Peer;

static Peer peers[MAX_PEERS];
static int peer_count = 0;

void load_peers(const char *config_path) {
    FILE *f = fopen(config_path, "r");
    if (!f) {
        printf("[!] No hay config de nodos en %s\n", config_path);
        return;
    }
    char line[128];
    while (fgets(line, sizeof(line), f) && peer_count < MAX_PEERS) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char ip[16], label[64];
        if (sscanf(line, "%15[^,],%63[^\n]", ip, label) == 2) {
            strcpy(peers[peer_count].ip, ip);
            strcpy(peers[peer_count].label, label);
            peer_count++;
        }
    }
    fclose(f);
    printf("[✓] %d nodos cargados\n", peer_count);
}

void log_alert(const char *type, const char *origin, const char *message) {
    FILE *f = fopen(ALERT_LOG, "a");
    if (!f) return;
    time_t t = time(NULL);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    fprintf(f, "[%s] tipo=%s origen=%s mensaje=%s\n", tbuf, type, origin, message);
    fflush(f);
    fsync(fileno(f));
    fclose(f);

    // Tambien queda en el ledger encadenado si esta disponible
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./bin/chrono-ledger append \"COMMUNITY_ALERT\" \"tipo=%s origen=%s\" 2>/dev/null", type, origin);
    system(cmd);
}

// Propaga la alerta a TODOS los nodos del mesh, sin depender de internet
void broadcast_alert(const char *alert_type, const char *message, const char *my_label) {
    char payload[512];
    snprintf(payload, sizeof(payload), "CHRONO_ALERT|%s|%s|%s", alert_type, my_label, message);

    int sent = 0, failed = 0;
    for (int i = 0; i < peer_count; i++) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ALERT_PORT);
        inet_pton(AF_INET, peers[i].ip, &addr.sin_addr);

        int r = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&addr, sizeof(addr));
        close(sock);
        if (r > 0) { sent++; printf("  [✓] Alerta enviada a %s (%s)\n", peers[i].label, peers[i].ip); }
        else { failed++; printf("  [!] No se pudo alcanzar %s (%s) - puede estar caido\n", peers[i].label, peers[i].ip); }
    }
    printf("[*] Propagacion: %d nodos alcanzados, %d no respondieron\n", sent, failed);
    log_alert(alert_type, my_label, message);
}

// Modo escucha: corre en cada nodo (garita, administracion, torres) esperando alertas
void listen_mode(const char *my_label) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(ALERT_PORT);
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    printf("[chrono-community-alert] Nodo '%s' escuchando alertas en puerto %d...\n", my_label, ALERT_PORT);
    log_alert("NODE_ONLINE", my_label, "nodo activo y escuchando");

    char buf[512];
    while (1) {
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&from, &fromlen);
        if (n <= 0) continue;
        buf[n] = '\0';

        char from_ip[16];
        inet_ntop(AF_INET, &from.sin_addr, from_ip, sizeof(from_ip));

        if (strncmp(buf, "CHRONO_ALERT|", 13) == 0) {
            char alert_type[64], origin[64], message[300];
            sscanf(buf + 13, "%63[^|]|%63[^|]|%299[^\n]", alert_type, origin, message);

            printf("\n");
            printf("==========================================\n");
            printf(" ALERTA RECIBIDA: %s\n", alert_type);
            printf(" Origen: %s (%s)\n", origin, from_ip);
            printf(" Mensaje: %s\n", message);
            printf("==========================================\n");

            log_alert(alert_type, origin, message);

            // Opcional: hacer sonar algo, encender un LED via GPIO, etc.
            // segun el hardware del nodo (garita puede tener altavoz/sirena)
            system("echo -e '\\a' 2>/dev/null"); // beep audible si el terminal lo soporta
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso:\n");
        printf("  chrono-community-alert listen <mi_etiqueta>\n");
        printf("      Modo escucha - corre esto en cada nodo (garita, torres, admin)\n\n");
        printf("  chrono-community-alert send <tipo> <mensaje> <mi_etiqueta>\n");
        printf("      Envia una alerta a todos los nodos del mesh\n");
        printf("      Tipos sugeridos: SISMO, INCENDIO, INTRUSION, MEDICA, CORTE_ENERGIA\n\n");
        printf("Requiere: etc/chrono/community_peers.conf con la lista de nodos\n");
        return 1;
    }

    load_peers("./etc/chrono/community_peers.conf");

    if (strcmp(argv[1], "listen") == 0 && argc >= 3) {
        listen_mode(argv[2]);
    } else if (strcmp(argv[1], "send") == 0 && argc >= 5) {
        broadcast_alert(argv[2], argv[3], argv[4]);
    } else {
        printf("[!] Argumentos invalidos\n");
        return 1;
    }
    return 0;
}
