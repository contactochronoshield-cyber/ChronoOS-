#ifndef CHRONO_CNEM_H
#define CHRONO_CNEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CHRONO_CNEM_VERSION "1.0.0"
#define CHRONO_CNEM_MAX_TEXT 128
#define CHRONO_CNEM_MAX_EVIDENCE 8

typedef enum {
    CHRONO_CNEM_UNKNOWN = 0,
    CHRONO_CNEM_INFO,
    CHRONO_CNEM_WARNING,
    CHRONO_CNEM_CRITICAL
} chrono_cnem_severity_t;

typedef enum {
    CHRONO_CNEM_DOMAIN_UNKNOWN = 0,
    CHRONO_CNEM_DOMAIN_NETWORK,
    CHRONO_CNEM_DOMAIN_ROUTING,
    CHRONO_CNEM_DOMAIN_RADIO,
    CHRONO_CNEM_DOMAIN_PON,
    CHRONO_CNEM_DOMAIN_SECURITY,
    CHRONO_CNEM_DOMAIN_SERVICE,
    CHRONO_CNEM_DOMAIN_SYSTEM
} chrono_cnem_domain_t;

typedef struct {
    char device_id[CHRONO_CNEM_MAX_TEXT];
    char vendor[CHRONO_CNEM_MAX_TEXT];
    char technology[CHRONO_CNEM_MAX_TEXT];
    char site[CHRONO_CNEM_MAX_TEXT];
    char interface_name[CHRONO_CNEM_MAX_TEXT];

    bool ipv4_ok;
    bool ipv6_ok;
    bool gateway_ok;
    bool dns_ok;
    bool routing_ok;
    bool bgp_ok;
    bool rpki_ok;
    bool pon_ok;
    bool radio_ok;

    double latency_ms;
    double jitter_ms;
    double packet_loss_percent;
} chrono_cnem_observation_t;

typedef struct {
    char service_id[CHRONO_CNEM_MAX_TEXT];
    char service_type[CHRONO_CNEM_MAX_TEXT];

    uint32_t affected_customers;
    uint32_t affected_devices;

    double sla_target_percent;
    double availability_percent;
} chrono_cnem_service_t;

typedef struct {
    char incident_id[CHRONO_CNEM_MAX_TEXT];
    chrono_cnem_severity_t severity;
    chrono_cnem_domain_t domain;

    char root_cause[CHRONO_CNEM_MAX_TEXT];
    char category[CHRONO_CNEM_MAX_TEXT];
    char origin[CHRONO_CNEM_MAX_TEXT];

    char evidence[CHRONO_CNEM_MAX_EVIDENCE][CHRONO_CNEM_MAX_TEXT];
    size_t evidence_count;

    uint32_t affected_customers;
    uint32_t affected_devices;
    uint32_t affected_services;

    bool sla_at_risk;
    bool operator_approval_required;
} chrono_cnem_incident_t;

bool chrono_cnem_validate_observation(
    const chrono_cnem_observation_t *observation
);

bool chrono_cnem_validate_service(
    const chrono_cnem_service_t *service
);

bool chrono_cnem_correlate(
    const chrono_cnem_observation_t *observation,
    const chrono_cnem_service_t *service,
    chrono_cnem_incident_t *incident
);

const char *chrono_cnem_severity_name(
    chrono_cnem_severity_t severity
);

const char *chrono_cnem_domain_name(
    chrono_cnem_domain_t domain
);

void chrono_cnem_print_incident(
    const chrono_cnem_incident_t *incident
);

#ifdef __cplusplus
}
#endif

#endif
