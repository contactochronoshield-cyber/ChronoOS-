/**
 * ChronoOS - Community Alert
 *
 * Sistema de alerta de emergencia para nodos Chrono conectados
 * mediante mesh local.
 *
 * Seguridad del transporte:
 *   - HMAC-SHA256
 *   - clave fuera del codigo fuente
 *   - nonce aleatorio de 128 bits
 *   - timestamp anti-replay
 *   - registro persistente de nonces utilizados
 *   - autorizacion por IP de peer
 *
 * La autenticacion protege la alerta contra falsificacion dentro
 * de la LAN/mesh. El sistema conserva la propagacion UDP original.
 *
 * La clave debe ser provisionada de forma segura y ser la misma
 * para los nodos que pertenezcan al mismo dominio de confianza.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include "../common/chrono_exec.h"

#define ALERT_PORT 9001
#define MAX_PEERS 32
#define MAX_ALERT 300
#define MAX_TYPE 64
#define MAX_LABEL 64
#define MAX_NONCE_HEX 33
#define HMAC_HEX_LEN 65

#define ALERT_KEY_FILE "./security/keys/community_alert.key"
#define ALERT_LOG "./var/logs/community_alerts.log"
#define NONCE_STATE "./var/lib/chrono/community_alert_nonces.log"

#define MAX_CLOCK_SKEW 300
#define MAX_SEEN_NONCES 4096

typedef struct {
    char ip[INET_ADDRSTRLEN];
    char label[MAX_LABEL];
} Peer;

static Peer peers[MAX_PEERS];
static size_t peer_count = 0;

static void secure_zero(void *ptr, size_t len)
{
    if (ptr != NULL)
        OPENSSL_cleanse(ptr, len);
}

static int load_key(unsigned char key[32])
{
    FILE *f = fopen(ALERT_KEY_FILE, "rb");
    if (!f)
        return -1;

    char hex[65];
    memset(hex, 0, sizeof(hex));

    if (fread(hex, 1, 64, f) != 64) {
        fclose(f);
        secure_zero(key, 32);
        return -1;
    }

    fclose(f);

    for (size_t i = 0; i < 32; i++) {
        unsigned int value;

        if (sscanf(hex + (i * 2), "%2x", &value) != 1) {
            secure_zero(key, 32);
            return -1;
        }

        key[i] = (unsigned char)value;
    }

    return 0;
}

static void bytes_to_hex(const unsigned char *in, size_t len,
                         char *out, size_t out_size)
{
    static const char hex[] = "0123456789abcdef";

    if (out_size < (len * 2U) + 1U)
        return;

    for (size_t i = 0; i < len; i++) {
        out[i * 2U] = hex[(in[i] >> 4U) & 0x0fU];
        out[i * 2U + 1U] = hex[in[i] & 0x0fU];
    }

    out[len * 2U] = '\0';
}

static int hex_to_bytes(const char *hex, unsigned char *out, size_t out_len)
{
    size_t hex_len;

    if (!hex || !out)
        return -1;

    hex_len = strlen(hex);

    if (hex_len != out_len * 2U)
        return -1;

    for (size_t i = 0; i < out_len; i++) {
        unsigned int value;

        if (sscanf(hex + (i * 2U), "%2x", &value) != 1)
            return -1;

        out[i] = (unsigned char)value;
    }

    return 0;
}

static int make_hmac(const unsigned char key[32],
                     const char *canonical,
                     unsigned char output[32])
{
    unsigned int output_len = 0;

    if (!HMAC(EVP_sha256(),
              key,
              32,
              (const unsigned char *)canonical,
              strlen(canonical),
              output,
              &output_len))
        return -1;

    return output_len == 32U ? 0 : -1;
}

static int peer_allowed(const char *ip)
{
    for (size_t i = 0; i < peer_count; i++) {
        if (strcmp(peers[i].ip, ip) == 0)
            return 1;
    }

    return 0;
}

static int nonce_seen(const char *nonce_hex)
{
    FILE *f = fopen(NONCE_STATE, "r");

    if (!f)
        return 0;

    char line[128];
    size_t checked = 0;

    while (fgets(line, sizeof(line), f) && checked < MAX_SEEN_NONCES) {
        line[strcspn(line, "\r\n")] = '\0';

        if (strcmp(line, nonce_hex) == 0) {
            fclose(f);
            return 1;
        }

        checked++;
    }

    fclose(f);
    return 0;
}

static int remember_nonce(const char *nonce_hex)
{
    FILE *f = fopen(NONCE_STATE, "a");

    if (!f)
        return -1;

    if (fprintf(f, "%s\n", nonce_hex) < 0) {
        fclose(f);
        return -1;
    }

    if (fflush(f) != 0) {
        fclose(f);
        return -1;
    }

    if (fsync(fileno(f)) != 0) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

static void log_alert(const char *type,
                      const char *origin,
                      const char *message)
{
    FILE *f;
    time_t now;
    struct tm tm_now;
    char tbuf[32];

    mkdir("./var", 0700);
    mkdir("./var/logs", 0700);
    mkdir("./var/lib", 0700);
    mkdir("./var/lib/chrono", 0700);

    f = fopen(ALERT_LOG, "a");
    if (f) {
        now = time(NULL);

        if (localtime_r(&now, &tm_now) != NULL)
            strftime(tbuf, sizeof(tbuf),
                     "%Y-%m-%d %H:%M:%S", &tm_now);
        else
            snprintf(tbuf, sizeof(tbuf), "unknown-time");

        fprintf(f,
                "[%s] tipo=%s origen=%s mensaje=%s\n",
                tbuf,
                type,
                origin,
                message);

        fflush(f);
        fsync(fileno(f));
        fclose(f);
    }

    (void)chrono_ledger_append(
        "COMMUNITY_ALERT",
        type);
}

static int build_packet(const unsigned char key[32],
                        const char *alert_type,
                        const char *message,
                        const char *my_label,
                        char *packet,
                        size_t packet_size)
{
    unsigned char nonce[16];
    unsigned char mac[32];
    char nonce_hex[33];
    char mac_hex[65];
    char canonical[768];
    time_t timestamp;

    if (!alert_type || !message || !my_label ||
        !packet || packet_size == 0)
        return -1;

    if (strchr(alert_type, '|') ||
        strchr(message, '|') ||
        strchr(my_label, '|'))
        return -1;

    if (RAND_bytes(nonce, sizeof(nonce)) != 1)
        return -1;

    bytes_to_hex(nonce, sizeof(nonce),
                 nonce_hex, sizeof(nonce_hex));

    timestamp = time(NULL);

    if (timestamp < 0)
        return -1;

    /*
     * Formato canonico:
     * CHRONO_ALERT_V1|timestamp|nonce|type|origin|message
     */
    if (snprintf(canonical, sizeof(canonical),
                 "CHRONO_ALERT_V1|%lld|%s|%s|%s|%s",
                 (long long)timestamp,
                 nonce_hex,
                 alert_type,
                 my_label,
                 message) >= (int)sizeof(canonical))
        return -1;

    if (make_hmac(key, canonical, mac) != 0)
        return -1;

    bytes_to_hex(mac, sizeof(mac),
                 mac_hex, sizeof(mac_hex));

    if (snprintf(packet, packet_size,
                 "%s|%s",
                 canonical,
                 mac_hex) >= (int)packet_size)
        return -1;

    secure_zero(nonce, sizeof(nonce));
    secure_zero(mac, sizeof(mac));

    return 0;
}

static int parse_and_verify_packet(
    const unsigned char key[32],
    const char *packet,
    const char *from_ip,
    char *alert_type,
    size_t alert_type_size,
    char *origin,
    size_t origin_size,
    char *message,
    size_t message_size)
{
    char work[1024];
    char *fields[7];
    char *cursor;
    char *saveptr = NULL;
    int field_count = 0;

    unsigned char received_mac[32];
    unsigned char calculated_mac[32];
    char canonical[768];

    long long timestamp;
    time_t now;

    if (!packet || !from_ip ||
        !alert_type || !origin || !message)
        return -1;

    if (!peer_allowed(from_ip))
        return -2;

    if (strlen(packet) >= sizeof(work))
        return -1;

    strcpy(work, packet);

    cursor = strtok_r(work, "|", &saveptr);

    while (cursor && field_count < 7) {
        fields[field_count++] = cursor;
        cursor = strtok_r(NULL, "|", &saveptr);
    }

    if (field_count != 7)
        return -1;

    if (strcmp(fields[0], "CHRONO_ALERT_V1") != 0)
        return -1;

    if (strlen(fields[2]) != 32U ||
        strlen(fields[6]) != 64U)
        return -1;

    if (hex_to_bytes(fields[2], received_mac, 16) != 0)
        return -1;

    /*
     * fields:
     * 0 version
     * 1 timestamp
     * 2 nonce
     * 3 type
     * 4 origin
     * 5 message
     * 6 hmac
     */

    errno = 0;
    timestamp = strtoll(fields[1], NULL, 10);

    if (errno != 0)
        return -1;

    now = time(NULL);

    if (now < 0)
        return -1;

    if (timestamp > (long long)now + MAX_CLOCK_SKEW ||
        timestamp < (long long)now - MAX_CLOCK_SKEW)
        return -3;

    if (nonce_seen(fields[2]))
        return -4;

    if (snprintf(canonical, sizeof(canonical),
                 "%s|%s|%s|%s|%s|%s",
                 fields[0],
                 fields[1],
                 fields[2],
                 fields[3],
                 fields[4],
                 fields[5]) >= (int)sizeof(canonical))
        return -1;

    if (hex_to_bytes(fields[6], received_mac, 32) != 0)
        return -1;

    if (make_hmac(key, canonical, calculated_mac) != 0) {
        secure_zero(received_mac, sizeof(received_mac));
        return -1;
    }

    if (CRYPTO_memcmp(received_mac,
                      calculated_mac,
                      32) != 0) {
        secure_zero(received_mac, sizeof(received_mac));
        secure_zero(calculated_mac, sizeof(calculated_mac));
        return -5;
    }

    if (remember_nonce(fields[2]) != 0) {
        secure_zero(received_mac, sizeof(received_mac));
        secure_zero(calculated_mac, sizeof(calculated_mac));
        return -1;
    }

    if (strlen(fields[3]) >= alert_type_size ||
        strlen(fields[4]) >= origin_size ||
        strlen(fields[5]) >= message_size) {
        secure_zero(received_mac, sizeof(received_mac));
        secure_zero(calculated_mac, sizeof(calculated_mac));
        return -1;
    }

    strcpy(alert_type, fields[3]);
    strcpy(origin, fields[4]);
    strcpy(message, fields[5]);

    secure_zero(received_mac, sizeof(received_mac));
    secure_zero(calculated_mac, sizeof(calculated_mac));

    return 0;
}

static void load_peers(const char *config_path)
{
    FILE *f = fopen(config_path, "r");

    if (!f) {
        printf("[!] No hay config de nodos en %s\n",
               config_path);
        return;
    }

    char line[128];

    while (fgets(line, sizeof(line), f) &&
           peer_count < MAX_PEERS) {

        if (line[0] == '#' || line[0] == '\n')
            continue;

        char ip[INET_ADDRSTRLEN];
        char label[MAX_LABEL];

        if (sscanf(line,
                   "%15[^,],%63[^\n]",
                   ip,
                   label) == 2) {

            struct in_addr addr;

            if (inet_pton(AF_INET, ip, &addr) != 1)
                continue;

            strncpy(peers[peer_count].ip,
                    ip,
                    sizeof(peers[peer_count].ip) - 1);

            peers[peer_count].ip[
                sizeof(peers[peer_count].ip) - 1] = '\0';

            strncpy(peers[peer_count].label,
                    label,
                    sizeof(peers[peer_count].label) - 1);

            peers[peer_count].label[
                sizeof(peers[peer_count].label) - 1] = '\0';

            peer_count++;
        }
    }

    fclose(f);

    printf("[✓] %zu nodos autorizados cargados\n",
           peer_count);
}

static void broadcast_alert(const unsigned char key[32],
                            const char *alert_type,
                            const char *message,
                            const char *my_label)
{
    char packet[1024];

    if (build_packet(key,
                     alert_type,
                     message,
                     my_label,
                     packet,
                     sizeof(packet)) != 0) {

        printf("[!] No se pudo autenticar la alerta\n");
        return;
    }

    int sent = 0;
    int failed = 0;

    for (size_t i = 0; i < peer_count; i++) {

        int sock = socket(AF_INET, SOCK_DGRAM, 0);

        if (sock < 0) {
            failed++;
            continue;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = htons(ALERT_PORT);

        if (inet_pton(AF_INET,
                      peers[i].ip,
                      &addr.sin_addr) != 1) {
            close(sock);
            failed++;
            continue;
        }

        ssize_t r = sendto(
            sock,
            packet,
            strlen(packet),
            0,
            (struct sockaddr *)&addr,
            sizeof(addr));

        close(sock);

        if (r > 0) {
            sent++;
            printf("  [✓] Alerta autenticada enviada a %s (%s)\n",
                   peers[i].label,
                   peers[i].ip);
        } else {
            failed++;
            printf("  [!] No se pudo alcanzar %s (%s)\n",
                   peers[i].label,
                   peers[i].ip);
        }
    }

    printf("[*] Propagacion: %d nodos alcanzados, %d no respondieron\n",
           sent,
           failed);

    log_alert(alert_type,
              my_label,
              message);
}

static void listen_mode(const unsigned char key[32],
                        const char *my_label)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("socket");
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(ALERT_PORT);

    if (bind(sock,
             (struct sockaddr *)&addr,
             sizeof(addr)) != 0) {

        perror("bind");
        close(sock);
        return;
    }

    printf("[chrono-community-alert] Nodo '%s' escuchando "
           "alertas autenticadas en puerto %d...\n",
           my_label,
           ALERT_PORT);

    log_alert("NODE_ONLINE",
              my_label,
              "nodo activo y escuchando");

    char buf[1024];

    while (1) {

        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);

        ssize_t n = recvfrom(
            sock,
            buf,
            sizeof(buf) - 1,
            0,
            (struct sockaddr *)&from,
            &fromlen);

        if (n <= 0)
            continue;

        buf[n] = '\0';

        char from_ip[INET_ADDRSTRLEN];

        if (!inet_ntop(AF_INET,
                       &from.sin_addr,
                       from_ip,
                       sizeof(from_ip)))
            continue;

        char alert_type[MAX_TYPE];
        char origin[MAX_LABEL];
        char message[MAX_ALERT];

        int result = parse_and_verify_packet(
            key,
            buf,
            from_ip,
            alert_type,
            sizeof(alert_type),
            origin,
            sizeof(origin),
            message,
            sizeof(message));

        if (result != 0) {

            const char *reason = "INVALID";

            if (result == -2)
                reason = "UNAUTHORIZED_PEER";
            else if (result == -3)
                reason = "TIMESTAMP_REJECTED";
            else if (result == -4)
                reason = "REPLAY_REJECTED";
            else if (result == -5)
                reason = "AUTHENTICATION_FAILED";

            printf("[BLOCK] Alerta UDP rechazada: %s desde %s\n",
                   reason,
                   from_ip);

            log_alert("COMMUNITY_ALERT_REJECTED",
                      from_ip,
                      reason);

            continue;
        }

        printf("\n");
        printf("==========================================\n");
        printf(" ALERTA AUTENTICADA: %s\n", alert_type);
        printf(" Origen: %s (%s)\n", origin, from_ip);
        printf(" Mensaje: %s\n", message);
        printf("==========================================\n");

        log_alert(alert_type,
                  origin,
                  message);

        /*
         * Beep directo: no shell.
         */
        (void)write(STDOUT_FILENO, "\a", 1);
    }

    close(sock);
}

int main(int argc, char *argv[])
{
    /*
     * El listener puede ejecutarse como servicio o con stdout
     * redirigido. Salida sin buffer para observabilidad inmediata.
     */
    setvbuf(stdout, NULL, _IONBF, 0);

    unsigned char key[32];

    if (argc < 2) {
        printf("Uso:\n");
        printf("  chrono-community-alert listen <mi_etiqueta>\n");
        printf("  chrono-community-alert send <tipo> <mensaje> <mi_etiqueta>\n");
        printf("\n");
        printf("Requiere clave externa:\n");
        printf("  %s\n", ALERT_KEY_FILE);
        return 1;
    }

    if (load_key(key) != 0) {
        printf("[!] No se pudo cargar la clave de autenticacion\n");
        printf("    Archivo: %s\n", ALERT_KEY_FILE);
        return 1;
    }

    load_peers("./etc/chrono/community_peers.conf");

    if (strcmp(argv[1], "listen") == 0 && argc >= 3) {

        listen_mode(key, argv[2]);

    } else if (strcmp(argv[1], "send") == 0 && argc >= 5) {

        broadcast_alert(key,
                        argv[2],
                        argv[3],
                        argv[4]);

    } else {

        printf("[!] Argumentos invalidos\n");
        secure_zero(key, sizeof(key));
        return 1;
    }

    secure_zero(key, sizeof(key));
    return 0;
}
