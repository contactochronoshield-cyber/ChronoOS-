#define _POSIX_C_SOURCE 200809L

#include "chrono_continuum.h"

#include <errno.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define IDENTITY_FILE "identity"
#define STATE_FILE "state"
#define EVENTS_FILE "events.log"

#define HASH_HEX_SIZE 65
#define LINE_SIZE 8192

static int ensure_layout(const char *root)
{
    struct stat st;

    if (root == NULL || root[0] == '\0')
        return -1;

    if (stat(root, &st) == 0) {
        if (!S_ISDIR(st.st_mode))
            return -1;
        return 0;
    }

    if (mkdir(root, 0700) == 0)
        return 0;

    if (errno == EEXIST)
        return 0;

    return -1;
}

static void sha256_hex(const char *data, char out[HASH_HEX_SIZE])
{
    unsigned char digest[SHA256_DIGEST_LENGTH];

    SHA256(
        (const unsigned char *)data,
        strlen(data),
        digest
    );

    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
        snprintf(out + (i * 2), 3, "%02x", digest[i]);

    out[64] = '\0';
}

static int random_hex(char *out, size_t bytes)
{
    int fd = open("/dev/urandom", O_RDONLY);

    if (fd < 0)
        return -1;

    unsigned char buffer[64];

    if (bytes > sizeof(buffer)) {
        close(fd);
        return -1;
    }

    ssize_t n = read(fd, buffer, bytes);

    close(fd);

    if (n != (ssize_t)bytes)
        return -1;

    for (size_t i = 0; i < bytes; i++)
        snprintf(out + (i * 2), 3, "%02x", buffer[i]);

    out[bytes * 2] = '\0';

    return 0;
}

static int now_iso(char *out, size_t size)
{
    time_t now = time(NULL);
    struct tm tm_value;

    if (now == (time_t)-1)
        return -1;

    if (gmtime_r(&now, &tm_value) == NULL)
        return -1;

    if (strftime(
            out,
            size,
            "%Y-%m-%dT%H:%M:%SZ",
            &tm_value
        ) == 0)
        return -1;

    return 0;
}

static int valid_field(const char *value)
{
    if (value == NULL || *value == '\0')
        return 0;

    if (strchr(value, '|') != NULL)
        return 0;

    if (strchr(value, '\n') != NULL)
        return 0;

    if (strchr(value, '\r') != NULL)
        return 0;

    return 1;
}

static int read_state_hash(
    const char *root,
    char out[HASH_HEX_SIZE]
)
{
    char path[1024];
    FILE *fp;

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        STATE_FILE
    );

    fp = fopen(path, "r");

    if (fp == NULL) {
        strcpy(out, "GENESIS");
        return 0;
    }

    char line[256];

    while (fgets(line, sizeof(line), fp) != NULL) {

        if (strncmp(line, "head_hash=", 10) == 0) {

            char *value = line + 10;

            value[strcspn(value, "\r\n")] = '\0';

            if (strlen(value) == 64) {

                strncpy(
                    out,
                    value,
                    HASH_HEX_SIZE
                );

                out[64] = '\0';

                fclose(fp);

                return 0;
            }
        }
    }

    fclose(fp);

    strcpy(out, "GENESIS");

    return 0;
}

static int count_events(const char *root)
{
    char path[1024];
    FILE *fp;
    char line[LINE_SIZE];
    int count = 0;

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        EVENTS_FILE
    );

    fp = fopen(path, "r");

    if (fp == NULL)
        return 0;

    while (fgets(line, sizeof(line), fp) != NULL)
        count++;

    fclose(fp);

    return count;
}

int continuum_init(const char *root)
{
    char path[1024];
    char identity[33];
    FILE *fp;

    if (root == NULL)
        return -1;

    if (ensure_layout(root) < 0)
        return -1;

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        IDENTITY_FILE
    );

    if (access(path, F_OK) != 0) {

        if (random_hex(identity, 16) < 0)
            return -1;

        fp = fopen(path, "w");

        if (fp == NULL)
            return -1;

        fprintf(
            fp,
            "continuum_version=%s\n",
            CHRONO_CONTINUUM_VERSION
        );

        fprintf(
            fp,
            "continuity_id=CHRONO-%s\n",
            identity
        );

        fprintf(
            fp,
            "created=%ld\n",
            (long)time(NULL)
        );

        fclose(fp);

        chmod(path, 0600);
    }

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        STATE_FILE
    );

    if (access(path, F_OK) != 0) {

        fp = fopen(path, "w");

        if (fp == NULL)
            return -1;

        fprintf(
            fp,
            "continuum_version=%s\n",
            CHRONO_CONTINUUM_VERSION
        );

        fprintf(
            fp,
            "state=INITIALIZED\n"
        );

        fprintf(
            fp,
            "sequence=0\n"
        );

        fprintf(
            fp,
            "head_hash=GENESIS\n"
        );

        fclose(fp);

        chmod(path, 0600);
    }

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        EVENTS_FILE
    );

    if (access(path, F_OK) != 0) {

        fp = fopen(path, "w");

        if (fp == NULL)
            return -1;

        fclose(fp);

        chmod(path, 0600);
    }

    printf("Chrono Continuum initialized\n");

    return 0;
}

int continuum_event(
    const char *root,
    const char *event_type,
    const char *actor,
    const char *details
)
{
    char path[1024];
    char previous_hash[HASH_HEX_SIZE];
    char timestamp[64];
    char canonical[LINE_SIZE];
    char hash[HASH_HEX_SIZE];

    FILE *fp;

    int sequence;

    if (!valid_field(event_type) ||
        !valid_field(actor) ||
        !valid_field(details))
        return -1;

    if (continuum_init(root) < 0)
        return -1;

    read_state_hash(
        root,
        previous_hash
    );

    sequence = count_events(root) + 1;

    if (now_iso(timestamp, sizeof(timestamp)) < 0)
        return -1;

    snprintf(
        canonical,
        sizeof(canonical),
        "%d|%s|%s|%s|%s|%s",
        sequence,
        timestamp,
        event_type,
        actor,
        details,
        previous_hash
    );

    sha256_hex(
        canonical,
        hash
    );

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        EVENTS_FILE
    );

    fp = fopen(path, "a");

    if (fp == NULL)
        return -1;

    fprintf(
        fp,
        "%d|%s|%s|%s|%s|%s|%s\n",
        sequence,
        timestamp,
        event_type,
        actor,
        details,
        previous_hash,
        hash
    );

    fflush(fp);

    fclose(fp);

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        STATE_FILE
    );

    fp = fopen(path, "w");

    if (fp == NULL)
        return -1;

    fprintf(
        fp,
        "continuum_version=%s\n",
        CHRONO_CONTINUUM_VERSION
    );

    fprintf(
        fp,
        "state=ACTIVE\n"
    );

    fprintf(
        fp,
        "sequence=%d\n",
        sequence
    );

    fprintf(
        fp,
        "head_hash=%s\n",
        hash
    );

    fclose(fp);

    printf("CONTINUUM_EVENT_COMMITTED\n");
    printf("sequence=%d\n", sequence);
    printf("type=%s\n", event_type);
    printf("actor=%s\n", actor);
    printf("hash=%s\n", hash);

    return 0;
}

int continuum_verify(const char *root)
{
    char path[1024];
    char line[LINE_SIZE];

    char previous_hash[HASH_HEX_SIZE] = "GENESIS";

    FILE *fp;

    int expected_sequence = 1;
    int verified = 0;

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        EVENTS_FILE
    );

    fp = fopen(path, "r");

    if (fp == NULL) {

        printf("CONTINUUM_VERIFY\n");
        printf("state=EMPTY\n");
        printf("events=0\n");

        return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {

        char *fields[7];
        char *cursor = line;
        char *token;
        int field_count = 0;

        char canonical[LINE_SIZE];
        char calculated[HASH_HEX_SIZE];

        line[strcspn(line, "\r\n")] = '\0';

        while (
            (token = strsep(&cursor, "|")) != NULL &&
            field_count < 7
        ) {
            fields[field_count++] = token;
        }

        if (field_count != 7) {

            printf(
                "VERIFY_FAILURE=INVALID_EVENT_FORMAT\n"
            );

            fclose(fp);

            return 1;
        }

        int sequence = atoi(fields[0]);

        if (sequence != expected_sequence) {

            printf(
                "VERIFY_FAILURE=SEQUENCE_GAP\n"
            );

            fclose(fp);

            return 1;
        }

        if (strcmp(
                fields[5],
                previous_hash
            ) != 0) {

            printf(
                "VERIFY_FAILURE=CHAIN_BREAK\n"
            );

            fclose(fp);

            return 1;
        }

        snprintf(
            canonical,
            sizeof(canonical),
            "%s|%s|%s|%s|%s|%s",
            fields[0],
            fields[1],
            fields[2],
            fields[3],
            fields[4],
            fields[5]
        );

        sha256_hex(
            canonical,
            calculated
        );

        if (strcmp(
                calculated,
                fields[6]
            ) != 0) {

            printf(
                "VERIFY_FAILURE=HASH_MISMATCH\n"
            );

            printf(
                "sequence=%d\n",
                sequence
            );

            fclose(fp);

            return 1;
        }

        strncpy(
            previous_hash,
            fields[6],
            HASH_HEX_SIZE
        );

        previous_hash[64] = '\0';

        expected_sequence++;
        verified++;
    }

    fclose(fp);

    printf("CONTINUUM_VERIFY\n");
    printf("state=VERIFIED\n");
    printf("events=%d\n", verified);
    printf("head_hash=%s\n", previous_hash);

    return 0;
}

int continuum_state(const char *root)
{
    char path[1024];
    FILE *fp;
    char line[512];

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        STATE_FILE
    );

    fp = fopen(path, "r");

    if (fp == NULL) {

        printf(
            "CONTINUUM_STATE=UNINITIALIZED\n"
        );

        return 1;
    }

    printf(
        "CHRONO CONTINUUM STATE\n"
    );

    while (
        fgets(line, sizeof(line), fp) != NULL
    )
        fputs(line, stdout);

    fclose(fp);

    return 0;
}

int continuum_passport(const char *root)
{
    char path[1024];
    FILE *fp;
    char line[512];

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        IDENTITY_FILE
    );

    fp = fopen(path, "r");

    if (fp == NULL) {

        printf(
            "CHRONO INFRASTRUCTURE PASSPORT\n"
        );

        printf(
            "status=UNINITIALIZED\n"
        );

        return 1;
    }

    printf(
        "========================================\n"
    );

    printf(
        " CHRONO INFRASTRUCTURE PASSPORT\n"
    );

    printf(
        "========================================\n"
    );

    while (
        fgets(line, sizeof(line), fp) != NULL
    )
        fputs(line, stdout);

    fclose(fp);

    printf(
        "events=%d\n",
        count_events(root)
    );

    return 0;
}

int main(int argc, char **argv)
{
    const char *command;
    const char *root = ".";

    if (argc < 2) {
        fprintf(
            stderr,
            "Usage: %s init|event|verify|state|passport [root] ...\n",
            argv[0]
        );
        return 2;
    }

    command = argv[1];

    if (strcmp(command, "init") == 0) {
        if (argc >= 3)
            root = argv[2];

        return continuum_init(root);
    }

    if (strcmp(command, "event") == 0) {
        if (argc != 6) {
            fprintf(
                stderr,
                "Usage: %s event ROOT TYPE ACTOR DETAILS\n",
                argv[0]
            );
            return 2;
        }

        return continuum_event(
            argv[2],
            argv[3],
            argv[4],
            argv[5]
        );
    }

    if (strcmp(command, "verify") == 0) {
        if (argc >= 3)
            root = argv[2];

        return continuum_verify(root);
    }

    if (strcmp(command, "state") == 0) {
        if (argc >= 3)
            root = argv[2];

        return continuum_state(root);
    }

    if (strcmp(command, "passport") == 0) {
        if (argc >= 3)
            root = argv[2];

        return continuum_passport(root);
    }

    fprintf(
        stderr,
        "Unknown command: %s\n",
        command
    );

    return 2;
}
