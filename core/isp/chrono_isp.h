#ifndef CHRONO_ISP_H
#define CHRONO_ISP_H

#define CHRONO_ISP_VERSION "1.0.0"

typedef enum {
    CHRONO_SEV_INFO = 0,
    CHRONO_SEV_WARNING = 1,
    CHRONO_SEV_CRITICAL = 2
} chrono_severity_t;

typedef struct {
    char node_id[64];
    char node_type[32];
    double latency_ms;
    double jitter_ms;
    double packet_loss_pct;
    double cpu_pct;
    double memory_pct;
    double utilization_pct;
    int bgp_up;
    int rpki_valid;
    int rpki_invalid;
    int ipv6_enabled;
    int pon_los;
    int pon_flapping;
    int onu_errors;
    int management_exposed;
    int configuration_changed;
    int certificate_expired;
    int affected_customers;
} chrono_isp_telemetry_t;

typedef struct {
    char incident_id[64];
    chrono_severity_t severity;
    char node_id[64];
    char category[64];
    char probable_origin[256];
    char evidence[512];
    char recommendation[512];
    int affected_customers;
} chrono_isp_incident_t;

int chrono_isp_validate(const chrono_isp_telemetry_t *t);

int chrono_isp_analyze(
    const chrono_isp_telemetry_t *t,
    chrono_isp_incident_t *incident
);

int chrono_isp_self_test(void);

void chrono_isp_print_report(
    const chrono_isp_telemetry_t *t,
    const chrono_isp_incident_t *incident
);

#endif
