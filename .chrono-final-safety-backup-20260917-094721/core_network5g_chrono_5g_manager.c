/*
 * ChronoOS 5G Sovereign Manager
 *
 * Defensive 5G subscriber and service management.
 *
 * Security requirements:
 * - No shell execution.
 * - No system().
 * - No popen().
 * - External arguments are passed as argv[] directly to child processes.
 * - Subscriber identifiers are validated before use.
 * - Ledger events are recorded through direct process execution.
 *
 * Designed for authorized private 5G infrastructure.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>

#define SUBSCRIBER_DB "./etc/chrono/5g_subscribers.conf"

#define OPEN5GS_DBCTL "open5gs-dbctl"
#define SYSTEMCTL "systemctl"

#define MAX_IMSI 32
#define MAX_KEY 128
#define MAX_OPC 128
#define MAX_DEVICE_LABEL 128

static int valid_token(const char *value, size_t max_len)
{
    if (value == NULL || *value == '\0') {
        return 0;
    }

    size_t len = strnlen(value, max_len + 1);

    if (len == 0 || len > max_len) {
        return 0;
    }

    /*
     * Tokens must never contain shell metacharacters.
     * Although we do not invoke a shell anymore, keeping this restriction
     * provides an additional input-integrity boundary.
     */
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)value[i];

        if (c < 0x20 || c == 0x7f) {
            return 0;
        }

        if (c == '"' || c == '\'' || c == '`' ||
            c == ';' || c == '|' || c == '&' ||
            c == '$' || c == '(' || c == ')' ||
            c == '<' || c == '>' || c == '\\') {
            return 0;
        }
    }

    return 1;
}

static int valid_imsi(const char *imsi)
{
    if (!valid_token(imsi, MAX_IMSI)) {
        return 0;
    }

    size_t len = strlen(imsi);

    /*
     * IMSI is normally 14-15 decimal digits.
     * Accept 1-15 digits here to avoid hard-coding a deployment-specific
     * numbering plan while still rejecting non-numeric input.
     */
    if (len < 1 || len > 15) {
        return 0;
    }

    for (size_t i = 0; i < len; i++) {
        if (imsi[i] < '0' || imsi[i] > '9') {
            return 0;
        }
    }

    return 1;
}

static int valid_secret_token(const char *value, size_t max_len)
{
    return valid_token(value, max_len);
}

/*
 * Execute a program directly.
 *
 * There is intentionally no shell involved.
 *
 * argv[0] = executable
 * argv[1..] = arguments
 */
static int run_direct(const char *program, char *const argv[])
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("[!] fork");
        return -1;
    }

    if (pid == 0) {
        execvp(program, argv);

        fprintf(stderr,
                "[!] No se pudo ejecutar %s: %s\n",
                program,
                strerror(errno));

        _exit(127);
    }

    int status = 0;

    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }

        perror("[!] waitpid");
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        fprintf(stderr,
                "[!] Proceso %s terminado por señal %d\n",
                program,
                WTERMSIG(status));

        return 128 + WTERMSIG(status);
    }

    return -1;
}

/*
 * Chrono Ledger integration without shell execution.
 */
static int log_ledger(const char *event_type, const char *details)
{
    if (!valid_token(event_type, 128) ||
        !valid_token(details, 512)) {
        fprintf(stderr,
                "[!] Evento Ledger rechazado: entrada invalida.\n");
        return -1;
    }

    char *const argv[] = {
        "./bin/chrono-ledger",
        "append",
        (char *)event_type,
        (char *)details,
        NULL
    };

    int rc = run_direct("./bin/chrono-ledger", argv);

    if (rc != 0) {
        fprintf(stderr,
                "[!] Ledger no pudo registrar el evento (rc=%d).\n",
                rc);
    }

    return rc;
}

static int append_local_subscriber(
    const char *imsi,
    const char *device_label)
{
    FILE *f = fopen(SUBSCRIBER_DB, "a");

    if (!f) {
        fprintf(stderr,
                "[!] No se pudo abrir %s: %s\n",
                SUBSCRIBER_DB,
                strerror(errno));
        return -1;
    }

    time_t t = time(NULL);
    struct tm tm_value;

    if (localtime_r(&t, &tm_value) == NULL) {
        fclose(f);
        fprintf(stderr, "[!] No se pudo obtener la hora local.\n");
        return -1;
    }

    char tbuf[32];

    if (strftime(
            tbuf,
            sizeof(tbuf),
            "%Y-%m-%dT%H:%M:%S",
            &tm_value) == 0) {

        fclose(f);
        fprintf(stderr, "[!] No se pudo formatear la hora.\n");
        return -1;
    }

    /*
     * Device label is validated so it cannot introduce malformed CSV
     * fields or control characters.
     */
    fprintf(
        f,
        "%s,%s,%s,ACTIVE\n",
        imsi,
        device_label,
        tbuf);

    if (fflush(f) != 0) {
        fclose(f);
        return -1;
    }

    if (fclose(f) != 0) {
        return -1;
    }

    return 0;
}

/*
 * Register subscriber in Open5GS.
 */
static int register_subscriber(
    const char *imsi,
    const char *device_label,
    const char *key,
    const char *opc)
{
    if (!valid_imsi(imsi)) {
        fprintf(stderr, "[!] IMSI invalido.\n");
        return 1;
    }

    if (!valid_token(device_label, MAX_DEVICE_LABEL)) {
        fprintf(stderr, "[!] Etiqueta de dispositivo invalida.\n");
        return 1;
    }

    if (!valid_secret_token(key, MAX_KEY)) {
        fprintf(stderr, "[!] Key invalida.\n");
        return 1;
    }

    if (!valid_secret_token(opc, MAX_OPC)) {
        fprintf(stderr, "[!] OPC invalido.\n");
        return 1;
    }

    printf(
        "[*] Registrando suscriptor 5G: IMSI=%s dispositivo=%s\n",
        imsi,
        device_label);

    if (append_local_subscriber(imsi, device_label) != 0) {
        fprintf(stderr,
                "[!] No se pudo guardar la referencia local.\n");
        return 1;
    }

    /*
     * IMPORTANT:
     * No shell is used.
     *
     * open5gs-dbctl receives each value as an independent argv element.
     */
    char *const argv[] = {
        (char *)OPEN5GS_DBCTL,
        "add",
        (char *)imsi,
        (char *)key,
        (char *)opc,
        NULL
    };

    printf("[*] Ejecutando open5gs-dbctl de forma directa.\n");

    int rc = run_direct(OPEN5GS_DBCTL, argv);

    if (rc != 0) {
        fprintf(stderr,
                "[!] open5gs-dbctl no disponible o fallo (rc=%d).\n",
                rc);

        /*
         * The local record is retained for forensic traceability.
         * Do not claim successful Open5GS registration.
         */
        char details[512];

        snprintf(
            details,
            sizeof(details),
            "imsi=%s dispositivo=%s open5gs_rc=%d",
            imsi,
            device_label,
            rc);

        log_ledger(
            "5G_SUBSCRIBER_REGISTER_ATTEMPT",
            details);

        return 1;
    }

    char details[512];

    snprintf(
        details,
        sizeof(details),
        "imsi=%s dispositivo=%s",
        imsi,
        device_label);

    log_ledger(
        "5G_SUBSCRIBER_REGISTERED",
        details);

    printf(
        "[✓] Suscriptor registrado y trazado en Ledger.\n");

    return 0;
}

static int revoke_subscriber(const char *imsi)
{
    if (!valid_imsi(imsi)) {
        fprintf(stderr, "[!] IMSI invalido.\n");
        return 1;
    }

    char *const argv[] = {
        (char *)OPEN5GS_DBCTL,
        "remove",
        (char *)imsi,
        NULL
    };

    printf(
        "[*] Revocando IMSI %s mediante ejecución directa.\n",
        imsi);

    int rc = run_direct(OPEN5GS_DBCTL, argv);

    if (rc != 0) {
        fprintf(stderr,
                "[!] No se pudo revocar el suscriptor (rc=%d).\n",
                rc);

        char details[256];

        snprintf(
            details,
            sizeof(details),
            "imsi=%s open5gs_rc=%d",
            imsi,
            rc);

        log_ledger(
            "5G_SUBSCRIBER_REVOKE_ATTEMPT",
            details);

        return 1;
    }

    log_ledger(
        "5G_SUBSCRIBER_REVOKED",
        imsi);

    printf(
        "[✓] Suscriptor %s revocado.\n",
        imsi);

    return 0;
}

/*
 * Stop only the explicitly configured Open5GS services.
 *
 * No shell and no broad "pkill -f" operation.
 */
static int stop_open5gs_service(const char *service)
{
    if (!valid_token(service, 64)) {
        return -1;
    }

    char *const argv[] = {
        (char *)SYSTEMCTL,
        "stop",
        (char *)service,
        NULL
    };

    return run_direct(SYSTEMCTL, argv);
}

static int panic_shutdown_5g(void)
{
    printf(
        "[!] Deteniendo nucleo 5G como parte del protocolo de panico...\n");

    int failures = 0;

    const char *services[] = {
        "open5gs-amfd",
        "open5gs-smfd",
        "open5gs-upfd"
    };

    for (size_t i = 0; i < sizeof(services) / sizeof(services[0]); i++) {
        int rc = stop_open5gs_service(services[i]);

        if (rc != 0) {
            failures++;
            fprintf(
                stderr,
                "[!] No se pudo detener %s (rc=%d).\n",
                services[i],
                rc);
        }
    }

    if (failures == 0) {
        log_ledger(
            "5G_CORE_PANIC_SHUTDOWN",
            "servicios Open5GS detenidos por protocolo de panico");

        printf("[✓] Nucleo 5G detenido.\n");
        return 0;
    }

    log_ledger(
        "5G_CORE_PANIC_SHUTDOWN_PARTIAL",
        "algunos servicios Open5GS no pudieron detenerse");

    fprintf(
        stderr,
        "[!] Apagado de emergencia incompleto: %d servicio(s) fallaron.\n",
        failures);

    return 1;
}

static void show_status(void)
{
    printf(
        "=== ChronoOS 5G Sovereign - Estado ===\n");

    /*
     * Direct systemctl query.
     *
     * We deliberately do not use:
     *   systemctl ... | head ... || echo ...
     *
     * There is no shell pipeline here.
     */
    char *const argv[] = {
        (char *)SYSTEMCTL,
        "is-active",
        "open5gs-amfd",
        NULL
    };

    int rc = run_direct(SYSTEMCTL, argv);

    if (rc == 0) {
        printf("[+] Open5GS AMF: ACTIVE\n");
    } else if (rc == 3) {
        printf("[-] Open5GS AMF: INACTIVE\n");
    } else {
        printf(
            "[i] No se pudo consultar Open5GS mediante systemd.\n");
    }

    FILE *f = fopen(SUBSCRIBER_DB, "r");

    if (f) {
        int count = 0;
        char line[256];

        while (fgets(line, sizeof(line), f)) {
            count++;
        }

        fclose(f);

        printf(
            "Suscriptores registrados localmente: %d\n",
            count);
    } else {
        printf(
            "Sin suscriptores registrados aun.\n");
    }
}

int main(int argc, char *argv[])
{
    if (mkdir("./etc", 0755) != 0 && errno != EEXIST) {
        perror("[!] mkdir ./etc");
        return 1;
    }

    if (mkdir("./etc/chrono", 0755) != 0 && errno != EEXIST) {
        perror("[!] mkdir ./etc/chrono");
        return 1;
    }

    if (argc < 2) {
        printf("Uso:\n");
        printf(
            "  chrono-5g-manager register <imsi> <dispositivo> <key> <opc>\n");
        printf(
            "  chrono-5g-manager revoke <imsi>\n");
        printf(
            "  chrono-5g-manager panic-shutdown\n");
        printf(
            "  chrono-5g-manager status\n");

        return 1;
    }

    if (strcmp(argv[1], "register") == 0 && argc == 6) {
        return register_subscriber(
            argv[2],
            argv[3],
            argv[4],
            argv[5]);
    }

    if (strcmp(argv[1], "revoke") == 0 && argc == 3) {
        return revoke_subscriber(argv[2]);
    }

    if (strcmp(argv[1], "panic-shutdown") == 0 && argc == 2) {
        return panic_shutdown_5g();
    }

    if (strcmp(argv[1], "status") == 0 && argc == 2) {
        show_status();
        return 0;
    }

    printf("[!] Argumentos invalidos.\n");
    return 1;
}
