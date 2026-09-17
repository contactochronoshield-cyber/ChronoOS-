/*
 * ChronoOS Sovereign Ledger
 *
 * JSONL forensic audit ledger.
 * - SHA-256 hash chaining
 * - File locking
 * - fsync()
 * - Structured records
 *
 *
 *
 * CLI:
 *   chrono-ledger append <event_type> <details>
 *   chrono-ledger verify
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define LEDGER_DIR  "./security/ledger"
#define LEDGER_PATH "./security/ledger/chrono_ledger.jsonl"

#define GENESIS_HASH \
"0000000000000000000000000000000000000000000000000000000000000000"

#define HASH_LEN 64U
#define HASH_STR_LEN 65U
#define MAX_EVENT 128U
#define MAX_DETAILS 1024U
#define MAX_ESCAPED (MAX_DETAILS * 2U + 1U)
#define MAX_LINE 4096U

static int valid_text(const char *s, size_t max_len)
{
    if (s == NULL || s[0] == '\0') {
        return 0;
    }

    size_t len = strlen(s);

    if (len >= max_len) {
        return 0;
    }

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)s[i];

        if (c < 0x20U) {
            return 0;
        }
    }

    return 1;
}

static int json_escape(const char *input, char *output, size_t output_size)
{
    if (input == NULL || output == NULL || output_size == 0U) {
        return -1;
    }

    size_t j = 0U;

    for (size_t i = 0U; input[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)input[i];

        if (c == '"' || c == '\\') {
            if (j + 2U >= output_size) {
                return -1;
            }

            output[j++] = '\\';
            output[j++] = (char)c;
        } else {
            if (j + 1U >= output_size) {
                return -1;
            }

            output[j++] = (char)c;
        }
    }

    output[j] = '\0';
    return 0;
}

static int sha256_hex(const char *input, char output[HASH_STR_LEN])
{
    if (input == NULL || output == NULL) {
        return -1;
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0U;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();

    if (ctx == NULL) {
        return -1;
    }

    int ok = 1;

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
        ok = 0;
    }

    if (ok && EVP_DigestUpdate(ctx, input, strlen(input)) != 1) {
        ok = 0;
    }

    if (ok &&
        EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
        ok = 0;
    }

    EVP_MD_CTX_free(ctx);

    if (!ok || digest_len != 32U) {
        return -1;
    }

    for (unsigned int i = 0U; i < digest_len; ++i) {
        int written = snprintf(
            output + (i * 2U),
            3U,
            "%02x",
            digest[i]
        );

        if (written != 2) {
            return -1;
        }
    }

    output[HASH_LEN] = '\0';
    return 0;
}

static int ensure_ledger_dir(void)
{
    if (mkdir("security", 0700) != 0 && errno != EEXIST) {
        return -1;
    }

    if (mkdir(LEDGER_DIR, 0700) != 0 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

static int get_timestamp(char output[32])
{
    time_t now = time(NULL);

    if (now == (time_t)-1) {
        return -1;
    }

    struct tm tm_value;

    if (localtime_r(&now, &tm_value) == NULL) {
        return -1;
    }

    if (strftime(
            output,
            32U,
            "%Y-%m-%dT%H:%M:%S%z",
            &tm_value) == 0U) {
        return -1;
    }

    return 0;
}

static int get_last_hash(FILE *file, char output[HASH_STR_LEN])
{
    strcpy(output, GENESIS_HASH);

    if (fseek(file, 0L, SEEK_SET) != 0) {
        return -1;
    }

    char line[MAX_LINE];
    char last_hash[HASH_STR_LEN];

    while (fgets(line, sizeof(line), file) != NULL) {
        char *marker = strstr(line, "\"hash\":\"");

        if (marker == NULL) {
            continue;
        }

        marker += 8;

        if (strlen(marker) < HASH_LEN) {
            continue;
        }

        memcpy(last_hash, marker, HASH_LEN);
        last_hash[HASH_LEN] = '\0';

        int valid = 1;

        for (size_t i = 0U; i < HASH_LEN; ++i) {
            char c = last_hash[i];

            if (!((c >= '0' && c <= '9') ||
                  (c >= 'a' && c <= 'f'))) {
                valid = 0;
                break;
            }
        }

        if (valid) {
            strcpy(output, last_hash);
        }
    }

    return 0;
}

static int build_record(
    const char *timestamp,
    const char *event,
    const char *details,
    const char *previous_hash,
    char *output,
    size_t output_size)
{
    char event_escaped[MAX_EVENT * 2U + 1U];
    char details_escaped[MAX_DETAILS * 2U + 1U];

    if (json_escape(
            event,
            event_escaped,
            sizeof(event_escaped)) != 0) {
        return -1;
    }

    if (json_escape(
            details,
            details_escaped,
            sizeof(details_escaped)) != 0) {
        return -1;
    }

    int written = snprintf(
        output,
        output_size,
        "{\"timestamp\":\"%s\",\"event\":\"%s\","
        "\"details\":\"%s\",\"previous_hash\":\"%s\"}",
        timestamp,
        event_escaped,
        details_escaped,
        previous_hash
    );

    if (written < 0 || (size_t)written >= output_size) {
        return -1;
    }

    return 0;
}

static int chrono_ledger_append(
    const char *event_type,
    const char *details)
{
    if (!valid_text(event_type, MAX_EVENT) ||
        !valid_text(details, MAX_DETAILS)) {
        fprintf(stderr, "[chrono-ledger] Datos invalidos.\n");
        return 2;
    }

    if (ensure_ledger_dir() != 0) {
        perror("[chrono-ledger] No se pudo crear security/ledger");
        return 3;
    }

    FILE *file = fopen(LEDGER_PATH, "a+");

    if (file == NULL) {
        perror("[chrono-ledger] No se pudo abrir el ledger");
        return 4;
    }

    int fd = fileno(file);

    if (flock(fd, LOCK_EX) != 0) {
        perror("[chrono-ledger] LOCK_EX fallo");
        fclose(file);
        return 5;
    }

    char previous_hash[HASH_STR_LEN];

    if (get_last_hash(file, previous_hash) != 0) {
        flock(fd, LOCK_UN);
        fclose(file);
        return 6;
    }

    char timestamp[32];

    if (get_timestamp(timestamp) != 0) {
        flock(fd, LOCK_UN);
        fclose(file);
        return 7;
    }

    char record[MAX_LINE];

    if (build_record(
            timestamp,
            event_type,
            details,
            previous_hash,
            record,
            sizeof(record)) != 0) {
        fprintf(stderr, "[chrono-ledger] Registro demasiado grande.\n");
        flock(fd, LOCK_UN);
        fclose(file);
        return 8;
    }

    char record_hash[HASH_STR_LEN];

    if (sha256_hex(record, record_hash) != 0) {
        fprintf(stderr, "[chrono-ledger] SHA-256 fallo.\n");
        flock(fd, LOCK_UN);
        fclose(file);
        return 9;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        perror("[chrono-ledger] SEEK_END fallo");
        flock(fd, LOCK_UN);
        fclose(file);
        return 10;
    }

    int written = fprintf(
        file,
        "%s,\"hash\":\"%s\"}\n",
        record,
        record_hash
    );

    if (written < 0) {
        perror("[chrono-ledger] Escritura fallo");
        flock(fd, LOCK_UN);
        fclose(file);
        return 11;
    }

    if (fflush(file) != 0) {
        perror("[chrono-ledger] fflush fallo");
        flock(fd, LOCK_UN);
        fclose(file);
        return 12;
    }

    if (fsync(fd) != 0) {
        perror("[chrono-ledger] fsync fallo");
        flock(fd, LOCK_UN);
        fclose(file);
        return 13;
    }

    flock(fd, LOCK_UN);
    fclose(file);

    printf("[OK] Ledger append: %s\n", event_type);
    printf("[OK] Hash: %s\n", record_hash);

    return 0;
}

static int extract_field(
    const char *line,
    const char *field,
    char *output,
    size_t output_size)
{
    char pattern[128];

    int n = snprintf(
        pattern,
        sizeof(pattern),
        "\"%s\":\"",
        field
    );

    if (n < 0 || (size_t)n >= sizeof(pattern)) {
        return -1;
    }

    const char *start = strstr(line, pattern);

    if (start == NULL) {
        return -1;
    }

    start += strlen(pattern);

    size_t j = 0U;

    while (start[j] != '\0') {
        if (start[j] == '"' &&
            (j == 0U || start[j - 1U] != '\\')) {
            break;
        }

        if (j + 1U >= output_size) {
            return -1;
        }

        output[j] = start[j];
        ++j;
    }

    if (start[j] != '"') {
        return -1;
    }

    output[j] = '\0';
    return 0;
}

static int chrono_ledger_verify(void)
{
    FILE *file = fopen(LEDGER_PATH, "r");

    if (file == NULL) {
        if (errno == ENOENT) {
            printf("[i] Ledger vacio, nada que verificar.\n");
            return 0;
        }

        perror("[chrono-ledger] No se pudo abrir ledger");
        return 2;
    }

    int fd = fileno(file);

    if (flock(fd, LOCK_SH) != 0) {
        perror("[chrono-ledger] LOCK_SH fallo");
        fclose(file);
        return 3;
    }

    char expected_previous[HASH_STR_LEN];
    strcpy(expected_previous, GENESIS_HASH);

    char line[MAX_LINE];

    unsigned long long entry_number = 0ULL;
    int broken = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        ++entry_number;

        size_t len = strlen(line);

        if (len == 0U || line[len - 1U] != '\n') {
            fprintf(
                stderr,
                "[!] Entrada %llu demasiado larga o truncada.\n",
                entry_number
            );
            broken = 1;
            continue;
        }

        line[len - 1U] = '\0';

        char timestamp[64];
        char event[MAX_EVENT * 2U + 1U];
        char details[MAX_DETAILS * 2U + 1U];
        char previous_hash[HASH_STR_LEN];
        char stored_hash[HASH_STR_LEN];

        if (extract_field(
                line,
                "timestamp",
                timestamp,
                sizeof(timestamp)) != 0 ||
            extract_field(
                line,
                "event",
                event,
                sizeof(event)) != 0 ||
            extract_field(
                line,
                "details",
                details,
                sizeof(details)) != 0 ||
            extract_field(
                line,
                "previous_hash",
                previous_hash,
                sizeof(previous_hash)) != 0 ||
            extract_field(
                line,
                "hash",
                stored_hash,
                sizeof(stored_hash)) != 0) {

            fprintf(
                stderr,
                "[!] Entrada %llu malformada.\n",
                entry_number
            );

            broken = 1;
            continue;
        }

        if (strcmp(previous_hash, expected_previous) != 0) {
            fprintf(
                stderr,
                "[!] Entrada %llu: previous_hash no coincide.\n",
                entry_number
            );

            broken = 1;
        }

        char canonical[MAX_LINE];

        if (build_record(
                timestamp,
                event,
                details,
                previous_hash,
                canonical,
                sizeof(canonical)) != 0) {

            fprintf(
                stderr,
                "[!] Entrada %llu: no se pudo reconstruir.\n",
                entry_number
            );

            broken = 1;
            continue;
        }

        char calculated_hash[HASH_STR_LEN];

        if (sha256_hex(
                canonical,
                calculated_hash) != 0) {

            broken = 1;
            continue;
        }

        if (strcmp(calculated_hash, stored_hash) != 0) {
            fprintf(
                stderr,
                "[!] Entrada %llu: HASH INVALIDO.\n",
                entry_number
            );

            broken = 1;
        }

        strcpy(expected_previous, stored_hash);
    }

    flock(fd, LOCK_UN);
    fclose(file);

    printf(
        "[chrono-ledger] %llu entradas verificadas.\n",
        entry_number
    );

    if (broken) {
        printf(
            "RESULTADO: CADENA COMPROMETIDA - "
            "NO CONFIAR EN ESTE HISTORIAL.\n"
        );

        return 1;
    }

    printf(
        "RESULTADO: CADENA INTEGRA - "
        "historial verificado sin alteraciones.\n"
    );

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(
            stderr,
            "Uso: chrono-ledger append <tipo> <detalle>\n"
            "     chrono-ledger verify\n"
        );

        return 1;
    }

    if (strcmp(argv[1], "append") == 0) {
        if (argc != 4) {
            fprintf(
                stderr,
                "Uso: chrono-ledger append <tipo> <detalle>\n"
            );

            return 1;
        }

        return chrono_ledger_append(
            argv[2],
            argv[3]
        );
    }

    if (strcmp(argv[1], "verify") == 0) {
        if (argc != 2) {
            fprintf(
                stderr,
                "Uso: chrono-ledger verify\n"
            );

            return 1;
        }

        return chrono_ledger_verify();
    }

    fprintf(stderr, "[!] Comando desconocido.\n");
    return 1;
}
