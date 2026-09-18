#ifndef CHRONO_ANTENNA_H
#define CHRONO_ANTENNA_H

#include <stddef.h>

#define CHRONO_ANTENNA_ID_MAX 64
#define CHRONO_ANTENNA_TECH_MAX 32
#define CHRONO_ANTENNA_OWNER_MAX 128
#define CHRONO_ANTENNA_STATUS_MAX 32

typedef struct {
    char id[CHRONO_ANTENNA_ID_MAX];
    double latitude;
    double longitude;
    char technology[CHRONO_ANTENNA_TECH_MAX];
    char owner[CHRONO_ANTENNA_OWNER_MAX];
    char status[CHRONO_ANTENNA_STATUS_MAX];
    int authorized_access;
    double signal;
    double noise;
    double latency_ms;
    double packet_loss;
    double temperature_c;
    unsigned long uptime_seconds;
} chrono_antenna_t;

int chrono_antenna_validate(const chrono_antenna_t *antenna);
int chrono_antenna_register(const char *registry,
                            const chrono_antenna_t *antenna);
int chrono_antenna_analyze(const chrono_antenna_t *antenna);
void chrono_antenna_print(const chrono_antenna_t *antenna);

#endif
