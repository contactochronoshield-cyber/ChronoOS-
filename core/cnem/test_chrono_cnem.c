#include "chrono_cnem.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    chrono_cnem_observation_t observation = {0};
    chrono_cnem_service_t service = {0};
    chrono_cnem_incident_t incident = {0};

    (void)snprintf(
        observation.device_id,
        sizeof(observation.device_id),
        "POP-03"
    );

    (void)snprintf(
        observation.vendor,
        sizeof(observation.vendor),
        "MULTIVENDOR"
    );

    (void)snprintf(
        observation.technology,
        sizeof(observation.technology),
        "BGP/IPv4/IPv6"
    );

    (void)snprintf(
        observation.site,
        sizeof(observation.site),
        "BOG-POP-03"
    );

    (void)snprintf(
        observation.interface_name,
        sizeof(observation.interface_name),
        "uplink0"
    );

    observation.ipv4_ok = true;
    observation.ipv6_ok = true;
    observation.gateway_ok = true;
    observation.dns_ok = true;
    observation.routing_ok = false;
    observation.bgp_ok = false;
    observation.rpki_ok = true;
    observation.pon_ok = true;
    observation.radio_ok = true;

    observation.latency_ms = 31.0;
    observation.jitter_ms = 4.0;
    observation.packet_loss_percent = 0.8;

    (void)snprintf(
        service.service_id,
        sizeof(service.service_id),
        "INTERNET-ACCESS"
    );

    (void)snprintf(
        service.service_type,
        sizeof(service.service_type),
        "BROADBAND"
    );

    service.affected_customers = 1842U;
    service.affected_devices = 1U;
    service.sla_target_percent = 99.90;
    service.availability_percent = 99.72;

    if (!chrono_cnem_validate_observation(&observation)) {
        fprintf(stderr, "[FAIL] observation validation\n");
        return 1;
    }

    if (!chrono_cnem_validate_service(&service)) {
        fprintf(stderr, "[FAIL] service validation\n");
        return 1;
    }

    if (!chrono_cnem_correlate(
            &observation,
            &service,
            &incident)) {
        fprintf(stderr, "[FAIL] correlation\n");
        return 1;
    }

    if (incident.domain != CHRONO_CNEM_DOMAIN_ROUTING ||
        strcmp(incident.root_cause, "ROUTING") != 0 ||
        incident.affected_customers != 1842U ||
        !incident.sla_at_risk ||
        incident.evidence_count < 2U) {
        fprintf(stderr, "[FAIL] correlation result\n");
        return 1;
    }

    chrono_cnem_print_incident(&incident);

    printf("\nSTATUS: CORRELATED\n");
    printf("[OK] Chrono Network Event Model self-test passed\n");

    return 0;
}
