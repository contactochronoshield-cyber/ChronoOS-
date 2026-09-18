#ifndef CHRONO_CUMBIA_H
#define CHRONO_CUMBIA_H

#include <stddef.h>

#define CHRONO_CUMBIA_VERSION "1.0.0"

typedef enum {
    CUMBIA_STATUS_UNKNOWN = 0,
    CUMBIA_STATUS_ONLINE,
    CUMBIA_STATUS_DEGRADED,
    CUMBIA_STATUS_BLOCKED,
    CUMBIA_STATUS_OFFLINE
} chrono_cumbia_status_t;

typedef enum {
    CUMBIA_ACTION_OBSERVE = 0,
    CUMBIA_ACTION_CONNECT,
    CUMBIA_ACTION_READ,
    CUMBIA_ACTION_ANALYZE,
    CUMBIA_ACTION_CHANGE,
    CUMBIA_ACTION_RESTART,
    CUMBIA_ACTION_ISOLATE
} chrono_cumbia_action_t;

typedef struct {
    char device_id[64];
    char device_type[32];
    char technology[32];

    int authorized;
    int management_reachable;
    int ip_reachable;
    int gateway_reachable;
    int dns_ok;
    int ipv4_ok;
    int ipv6_ok;
    int routing_ok;
    int bgp_ok;
    int rpki_ok;
    int certificate_ok;

    double latency_ms;
    double jitter_ms;
    double packet_loss_pct;

    int config_changed;
    int authentication_failure;
    int interface_down;

    int affected_devices;
} chrono_cumbia_device_t;

typedef struct {
    chrono_cumbia_status_t status;
    int anomaly_count;
    int requires_operator;
    char incident_id[96];
    char category[64];
    char origin[128];
    char evidence[512];
    char recommendation[512];
} chrono_cumbia_result_t;

int chrono_cumbia_validate(const chrono_cumbia_device_t *device);

int chrono_cumbia_authorized_action(
    const chrono_cumbia_device_t *device,
    chrono_cumbia_action_t action
);

int chrono_cumbia_analyze(
    const chrono_cumbia_device_t *device,
    chrono_cumbia_result_t *result
);

void chrono_cumbia_print_report(
    const chrono_cumbia_device_t *device,
    const chrono_cumbia_result_t *result
);

#endif
