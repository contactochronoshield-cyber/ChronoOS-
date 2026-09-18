#ifndef CHRONO_ASSURANCE_H
#define CHRONO_ASSURANCE_H

#ifdef __cplusplus
extern "C" {
#endif

#define CHRONO_ASSURANCE_VERSION "1.0.0"

typedef enum {
    CHRONO_ASSURANCE_OK = 0,
    CHRONO_ASSURANCE_DEGRADED,
    CHRONO_ASSURANCE_CRITICAL,
    CHRONO_ASSURANCE_UNKNOWN
} chrono_assurance_status_t;

typedef enum {
    CHRONO_ROOT_NONE = 0,
    CHRONO_ROOT_LINK,
    CHRONO_ROOT_ROUTING,
    CHRONO_ROOT_DNS,
    CHRONO_ROOT_AUTH,
    CHRONO_ROOT_PON,
    CHRONO_ROOT_RADIO,
    CHRONO_ROOT_CAPACITY,
    CHRONO_ROOT_CONFIGURATION,
    CHRONO_ROOT_CERTIFICATE,
    CHRONO_ROOT_MULTI_DOMAIN
} chrono_root_cause_t;

typedef struct {
    char device_id[64];
    char site_id[64];
    char service_id[64];

    int management_ok;
    int interface_ok;
    int gateway_ok;
    int dns_ok;
    int ipv4_ok;
    int ipv6_ok;
    int routing_ok;
    int bgp_ok;
    int rpki_ok;
    int certificate_ok;
    int pon_ok;
    int radio_ok;
    int configuration_changed;

    double latency_ms;
    double jitter_ms;
    double packet_loss_pct;

    unsigned int affected_customers;
    unsigned int affected_services;

    double sla_target_pct;
    double current_availability_pct;
} chrono_assurance_observation_t;

typedef struct {
    chrono_assurance_status_t status;
    chrono_root_cause_t root_cause;

    unsigned int evidence_count;
    unsigned int affected_customers;
    unsigned int affected_services;

    int sla_at_risk;
    int requires_operator;
    int rollback_available;

    char incident_id[96];
    char category[64];
    char origin[256];
    char evidence[1024];
    char recommendation[512];
} chrono_assurance_result_t;

int chrono_assurance_validate(
    const chrono_assurance_observation_t *observation
);

int chrono_assurance_analyze(
    const chrono_assurance_observation_t *observation,
    chrono_assurance_result_t *result
);

const char *chrono_assurance_status_name(
    chrono_assurance_status_t status
);

const char *chrono_assurance_root_name(
    chrono_root_cause_t root
);

void chrono_assurance_print_report(
    const chrono_assurance_observation_t *observation,
    const chrono_assurance_result_t *result
);

#ifdef __cplusplus
}
#endif

#endif
