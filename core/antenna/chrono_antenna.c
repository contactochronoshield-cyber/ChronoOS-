#include "chrono_antenna.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int safe_text(const char *s, size_t max)
{
    if (s == NULL || s[0] == '\0') {
        return 0;
    }

    if (strlen(s) >= max) {
        return 0;
    }

    if (strchr(s, ',') != NULL) {
        return 0;
    }

    return 1;
}

int chrono_antenna_validate(const chrono_antenna_t *a)
{
    if (a == NULL) {
        return 0;
    }

    if (!safe_text(a->id, sizeof(a->id))) {
        return 0;
    }

    if (!safe_text(a->technology, sizeof(a->technology))) {
        return 0;
    }

    if (!safe_text(a->owner, sizeof(a->owner))) {
        return 0;
    }

    if (a->latitude < -90.0 || a->latitude > 90.0) {
        return 0;
    }

    if (a->longitude < -180.0 || a->longitude > 180.0) {
        return 0;
    }

    if (a->authorized_access != 0 && a->authorized_access != 1) {
        return 0;
    }

    return 1;
}

int chrono_antenna_register(const char *registry,
                            const chrono_antenna_t *a)
{
    FILE *fp;

    if (registry == NULL || !chrono_antenna_validate(a)) {
        return -1;
    }

    fp = fopen(registry, "a+");
    if (fp == NULL) {
        return -1;
    }

    if (ftell(fp) == 0) {
        fprintf(fp,
                "id,latitude,longitude,technology,owner,status,"
                "authorized_access,signal,noise,latency_ms,"
                "packet_loss,temperature_c,uptime_seconds\n");
    }

    fprintf(fp,
            "%s,%.8f,%.8f,%s,%s,%s,%d,%.3f,%.3f,%.3f,%.5f,%.2f,%lu\n",
            a->id,
            a->latitude,
            a->longitude,
            a->technology,
            a->owner,
            a->status,
            a->authorized_access,
            a->signal,
            a->noise,
            a->latency_ms,
            a->packet_loss,
            a->temperature_c,
            a->uptime_seconds);

    fclose(fp);
    return 0;
}

int chrono_antenna_analyze(const chrono_antenna_t *a)
{
    int warnings = 0;

    if (!chrono_antenna_validate(a)) {
        return -1;
    }

    printf("\n=== CHRONO ANTENNA INTELLIGENCE ===\n");
    printf("ID:           %s\n", a->id);
    printf("Technology:   %s\n", a->technology);
    printf("Owner:        %s\n", a->owner);
    printf("Coordinates:  %.6f, %.6f\n",
           a->latitude, a->longitude);
    printf("Status:       %s\n", a->status);
    printf("Access:       %s\n",
           a->authorized_access ? "AUTHORIZED" : "PUBLIC/NO MANAGEMENT ACCESS");

    printf("\n--- RADIO ---\n");
    printf("Signal:       %.2f\n", a->signal);
    printf("Noise:        %.2f\n", a->noise);

    if (a->signal < -110.0) {
        printf("WARNING: weak signal metric\n");
        warnings++;
    }

    if (a->noise > -80.0) {
        printf("WARNING: elevated noise metric\n");
        warnings++;
    }

    printf("\n--- NETWORK ---\n");
    printf("Latency:      %.3f ms\n", a->latency_ms);
    printf("Packet loss:  %.5f %%\n", a->packet_loss);

    if (a->latency_ms > 100.0) {
        printf("WARNING: high latency\n");
        warnings++;
    }

    if (a->packet_loss > 2.0) {
        printf("WARNING: packet loss elevated\n");
        warnings++;
    }

    printf("\n--- HARDWARE ---\n");
    printf("Temperature:  %.2f C\n", a->temperature_c);
    printf("Uptime:       %lu seconds\n", a->uptime_seconds);

    if (a->temperature_c > 80.0) {
        printf("WARNING: high temperature\n");
        warnings++;
    }

    printf("\n--- SECURITY BOUNDARY ---\n");

    if (a->authorized_access) {
        printf("Management analysis: AUTHORIZED\n");
        printf("External access:     NOT performed by CAI\n");
    } else {
        printf("Management analysis: NOT AUTHORIZED\n");
        printf("Analysis limited to supplied/public data.\n");
    }

    printf("\n--- RESULT ---\n");

    if (warnings == 0) {
        printf("STATUS: OPERATIONAL\n");
    } else {
        printf("STATUS: ATTENTION REQUIRED\n");
        printf("Warnings: %d\n", warnings);
    }

    return warnings;
}

void chrono_antenna_print(const chrono_antenna_t *a)
{
    if (a == NULL) {
        return;
    }

    printf("%s | %.6f,%.6f | %s | %s | %s | access=%d\n",
           a->id,
           a->latitude,
           a->longitude,
           a->technology,
           a->owner,
           a->status,
           a->authorized_access);
}

static void usage(const char *program)
{
    printf("Chrono Antenna Intelligence\n\n");
    printf("Usage:\n");
    printf("  %s --help\n", program);
    printf("  %s --self-test\n", program);
    printf("  %s --register ID LAT LON TECH OWNER [AUTHORIZED]\n",
           program);
    printf("  %s --analyze ID LAT LON TECH OWNER SIGNAL NOISE "
           "LATENCY LOSS TEMP UPTIME [AUTHORIZED]\n",
           program);
}

static int parse_double(const char *s, double *out)
{
    char *end = NULL;
    double value;

    errno = 0;
    value = strtod(s, &end);

    if (errno != 0 || end == s || *end != '\0') {
        return 0;
    }

    *out = value;
    return 1;
}

static int parse_ulong(const char *s, unsigned long *out)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(s, &end, 10);

    if (errno != 0 || end == s || *end != '\0') {
        return 0;
    }

    *out = value;
    return 1;
}

int main(int argc, char **argv)
{
    chrono_antenna_t a;
    const char *registry = "var/antenna/registry.csv";

    memset(&a, 0, sizeof(a));

    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        usage(argv[0]);
        return argc < 2 ? 1 : 0;
    }

    if (strcmp(argv[1], "--self-test") == 0) {
        strcpy(a.id, "CAI-SELFTEST");
        strcpy(a.technology, "TEST");
        strcpy(a.owner, "CHRONO");
        strcpy(a.status, "ONLINE");

        a.latitude = 4.711000;
        a.longitude = -74.072000;
        a.authorized_access = 1;
        a.signal = -70.0;
        a.noise = -100.0;
        a.latency_ms = 10.0;
        a.packet_loss = 0.01;
        a.temperature_c = 40.0;
        a.uptime_seconds = 3600;

        if (!chrono_antenna_validate(&a)) {
            fprintf(stderr, "[FAIL] antenna validation\n");
            return 1;
        }

        printf("[OK] antenna validation\n");
        chrono_antenna_analyze(&a);
        return 0;
    }

    if (strcmp(argv[1], "--register") == 0) {
        if (argc < 7 || argc > 8) {
            usage(argv[0]);
            return 1;
        }

        strncpy(a.id, argv[2], sizeof(a.id) - 1);
        strncpy(a.technology, argv[5], sizeof(a.technology) - 1);
        strncpy(a.owner, argv[6], sizeof(a.owner) - 1);
        strcpy(a.status, "REGISTERED");

        if (!parse_double(argv[3], &a.latitude) ||
            !parse_double(argv[4], &a.longitude)) {
            fprintf(stderr, "[FAIL] invalid coordinates\n");
            return 1;
        }

        a.authorized_access = 0;

        if (argc == 8) {
            a.authorized_access = atoi(argv[7]) != 0;
        }

        if (!chrono_antenna_validate(&a)) {
            fprintf(stderr, "[FAIL] invalid antenna\n");
            return 1;
        }

        if (chrono_antenna_register(registry, &a) != 0) {
            fprintf(stderr, "[FAIL] registry write\n");
            return 1;
        }

        printf("[OK] antenna registered: %s\n", a.id);
        return 0;
    }

    if (strcmp(argv[1], "--analyze") == 0) {
        if (argc < 13 || argc > 14) {
            usage(argv[0]);
            return 1;
        }

        strncpy(a.id, argv[2], sizeof(a.id) - 1);
        strncpy(a.technology, argv[5], sizeof(a.technology) - 1);
        strncpy(a.owner, argv[6], sizeof(a.owner) - 1);
        strcpy(a.status, "ONLINE");

        if (!parse_double(argv[3], &a.latitude) ||
            !parse_double(argv[4], &a.longitude) ||
            !parse_double(argv[7], &a.signal) ||
            !parse_double(argv[8], &a.noise) ||
            !parse_double(argv[9], &a.latency_ms) ||
            !parse_double(argv[10], &a.packet_loss) ||
            !parse_double(argv[11], &a.temperature_c) ||
            !parse_ulong(argv[12], &a.uptime_seconds)) {
            fprintf(stderr, "[FAIL] invalid metric\n");
            return 1;
        }

        a.authorized_access = 0;

        if (argc == 14) {
            a.authorized_access = atoi(argv[13]) != 0;
        }

        return chrono_antenna_analyze(&a) < 0 ? 1 : 0;
    }

    usage(argv[0]);
    return 1;
}
