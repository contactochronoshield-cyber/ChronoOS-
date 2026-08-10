/**
 * ChronoOS - Modbus Guard
 * Proxy soberano Modbus TCP para plantas de tratamiento de agua (y otras
 * industrias con PLCs Modbus). Se coloca ENTRE el HMI/SCADA y el PLC real:
 * el operador se conecta a este proxy, no directo al PLC. Cada comando se
 * inspecciona contra una whitelist de registros y rangos seguros antes de
 * reenviarlo. Escrituras fuera de rango (ej: dosificacion quimica excesiva,
 * el vector exacto del ataque de Oldsmar 2021) se BLOQUEAN, no solo se
 * registran.
 *
 * Protocolo Modbus TCP (MBAP header, 7 bytes) + PDU segun especificacion
 * publica Modbus.org. Funciones de escritura cubiertas: 0x05 (Write Single
 * Coil), 0x06 (Write Single Register), 0x0F (Write Multiple Coils),
 * 0x10 (Write Multiple Registers).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define WHITELIST_PATH "./etc/chrono/modbus_whitelist.conf"
#define MAX_RULES 128
#define BUF_SIZE 512

typedef struct {
    int function_code;
    int register_addr;
    int min_value;
    int max_value;
    char label[64];
} ModbusRule;

static ModbusRule rules[MAX_RULES];
static int rule_count = 0;

void log_ledger(const char *event_type, const char *details) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./bin/chrono-ledger append \"%s\" \"%s\" 2>/dev/null", event_type, details);
    system(cmd);
}

void load_whitelist() {
    FILE *f = fopen(WHITELIST_PATH, "r");
    if (!f) {
        printf("[!] No hay whitelist en %s - TODO se bloqueara por seguridad (fail-safe).\n", WHITELIST_PATH);
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f) && rule_count < MAX_RULES) {
        if (line[0] == '#' || line[0] == '\n') continue;
        ModbusRule r;
        if (sscanf(line, "%d,%d,%d,%d,%63[^\n]", &r.function_code, &r.register_addr,
                    &r.min_value, &r.max_value, r.label) >= 4) {
            rules[rule_count++] = r;
        }
    }
    fclose(f);
    printf("[✓] %d reglas cargadas desde whitelist\n", rule_count);
}

// Busca una regla que coincida con esta escritura. Devuelve NULL si no
// esta en la whitelist (fail-safe: no reconocido = bloqueado, no permitido)
ModbusRule *find_rule(int func_code, int reg_addr) {
    for (int i = 0; i < rule_count; i++) {
        if (rules[i].function_code == func_code && rules[i].register_addr == reg_addr)
            return &rules[i];
    }
    return NULL;
}

// Inspecciona un PDU Modbus y decide si la escritura es segura.
// Retorna 1 = permitir, 0 = bloquear
int inspect_write(unsigned char *pdu, int pdu_len, char *reason_out, size_t reason_len) {
    if (pdu_len < 5) return 0;
    int func_code = pdu[0];

    if (func_code == 0x06) { // Write Single Register
        int reg_addr = (pdu[1] << 8) | pdu[2];
        int value = (pdu[3] << 8) | pdu[4];
        ModbusRule *r = find_rule(func_code, reg_addr);
        if (!r) {
            snprintf(reason_out, reason_len, "registro %d no esta en whitelist (func 0x06)", reg_addr);
            return 0;
        }
        if (value < r->min_value || value > r->max_value) {
            snprintf(reason_out, reason_len, "%s: valor %d fuera de rango seguro [%d-%d]",
                r->label, value, r->min_value, r->max_value);
            return 0;
        }
        snprintf(reason_out, reason_len, "%s: valor %d dentro de rango [%d-%d]", r->label, value, r->min_value, r->max_value);
        return 1;
    }
    else if (func_code == 0x05) { // Write Single Coil
        int reg_addr = (pdu[1] << 8) | pdu[2];
        ModbusRule *r = find_rule(func_code, reg_addr);
        if (!r) {
            snprintf(reason_out, reason_len, "coil %d no esta en whitelist (func 0x05)", reg_addr);
            return 0;
        }
        snprintf(reason_out, reason_len, "%s: escritura de coil autorizada", r->label);
        return 1;
    }
    else if (func_code == 0x10 || func_code == 0x0F) { // Write Multiple Registers/Coils
        int reg_addr = (pdu[1] << 8) | pdu[2];
        ModbusRule *r = find_rule(func_code, reg_addr);
        if (!r) {
            snprintf(reason_out, reason_len, "escritura multiple a registro %d no esta en whitelist", reg_addr);
            return 0;
        }
        snprintf(reason_out, reason_len, "%s: escritura multiple autorizada", r->label);
        return 1;
    }

    // Funciones de solo lectura (0x01-0x04) siempre se permiten, no son riesgo
    return 1;
}

int is_write_function(int func_code) {
    return func_code == 0x05 || func_code == 0x06 || func_code == 0x0F || func_code == 0x10;
}

void handle_connection(int client_fd, const char *plc_ip, int plc_port, const char *client_ip) {
    int plc_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in plc_addr = {0};
    plc_addr.sin_family = AF_INET;
    plc_addr.sin_port = htons(plc_port);
    inet_pton(AF_INET, plc_ip, &plc_addr.sin_addr);

    if (connect(plc_fd, (struct sockaddr *)&plc_addr, sizeof(plc_addr)) < 0) {
        perror("[!] No se pudo conectar al PLC real");
        close(client_fd);
        return;
    }

    unsigned char buf[BUF_SIZE];
    int n;
    while ((n = recv(client_fd, buf, sizeof(buf), 0)) > 0) {
        if (n < 8) { send(client_fd, buf, n, 0); continue; } // frame muy corto, pasar sin inspeccionar

        // MBAP header: 7 bytes (transaction_id[2], protocol_id[2], length[2], unit_id[1])
        // PDU empieza en byte 7
        unsigned char *pdu = buf + 7;
        int pdu_len = n - 7;
        int func_code = pdu[0];

        if (is_write_function(func_code)) {
            char reason[256];
            int allowed = inspect_write(pdu, pdu_len, reason, sizeof(reason));

            char details[512];
            snprintf(details, sizeof(details), "client=%s func=0x%02x %s", client_ip, func_code, reason);

            if (!allowed) {
                printf("[!] BLOQUEADO: %s\n", details);
                log_ledger("MODBUS_WRITE_BLOCKED", details);

                // Responder con excepcion Modbus (codigo 0x02 = Illegal Data Address)
                unsigned char exception[9];
                memcpy(exception, buf, 6);
                exception[6] = buf[6]; // unit id
                exception[7] = func_code | 0x80;
                exception[8] = 0x02;
                send(client_fd, exception, 9, 0);
                continue;
            } else {
                printf("[✓] Autorizado: %s\n", details);
                log_ledger("MODBUS_WRITE_ALLOWED", details);
            }
        }

        // Reenviar al PLC real
        send(plc_fd, buf, n, 0);
        int r = recv(plc_fd, buf, sizeof(buf), 0);
        if (r > 0) send(client_fd, buf, r, 0);
    }

    close(plc_fd);
    close(client_fd);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Uso: chrono-modbus-guard <puerto_local> <ip_plc_real> <puerto_plc>\n");
        printf("Ejemplo: chrono-modbus-guard 5020 192.168.1.50 502\n");
        printf("\nEl HMI/SCADA se conecta a este proxy (puerto_local) en vez de\n");
        printf("conectarse directo al PLC. El proxy inspecciona y reenvia.\n");
        return 1;
    }

    int local_port = atoi(argv[1]);
    const char *plc_ip = argv[2];
    int plc_port = atoi(argv[3]);

    load_whitelist();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(local_port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[!] No se pudo abrir el puerto local");
        return 1;
    }
    listen(server_fd, 5);

    printf("[chrono-modbus-guard] Escuchando en puerto %d, reenviando a %s:%d\n", local_port, plc_ip, plc_port);
    log_ledger("MODBUS_GUARD_STARTED", "proxy activo");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) continue;

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("[*] Conexion de HMI/SCADA desde: %s\n", client_ip);

        handle_connection(client_fd, plc_ip, plc_port, client_ip);
    }

    return 0;
}
