#include "chrono_assurance.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    chrono_assurance_observation_t observation;
    chrono_assurance_result_t result;

    if (argc == 2 && strcmp(argv[1], "--self-test") != 0) {
        fprintf(stderr, "Uso: %s --self-test\n", argv[0]);
        return 1;
    }

    memset(&observation, 0, sizeof(observation));

    snprintf(observation.device_id,
             sizeof(observation.device_id),
             "POP-03");

    snprintf(observation.site_id,
             sizeof(observation.site_id),
             "BOG-POP-03");

    snprintf(observation.service_id,
             sizeof(observation.service_id),
             "INTERNET-ACCESS");

    observation.management_ok = 1;
    observation.interface_ok = 1;
    observation.gateway_ok = 1;
    observation.dns_ok = 1;
    observation.ipv4_ok = 1;
    observation.ipv6_ok = 1;
    observation.routing_ok = 0;
    observation.bgp_ok = 0;
    observation.rpki_ok = 1;
    observation.certificate_ok = 1;
    observation.pon_ok = 1;
    observation.radio_ok = 1;
    observation.configuration_changed = 1;

    observation.latency_ms = 31.0;
    observation.jitter_ms = 4.0;
    observation.packet_loss_pct = 0.8;

    observation.affected_customers = 1842;
    observation.affected_services = 3;

    observation.sla_target_pct = 99.90;
    observation.current_availability_pct = 99.72;

    if (!chrono_assurance_analyze(&observation, &result)) {
        fprintf(stderr, "[FAIL] assurance analysis\n");
        return 1;
    }

    chrono_assurance_print_report(&observation, &result);

    if (result.status != CHRONO_ASSURANCE_DEGRADED) {
        fprintf(stderr, "[FAIL] expected DEGRADED\n");
        return 1;
    }

    if (result.root_cause != CHRONO_ROOT_ROUTING) {
        fprintf(stderr, "[FAIL] expected ROUTING root cause\n");
        return 1;
    }

    if (result.affected_customers != 1842U) {
        fprintf(stderr, "[FAIL] impact calculation\n");
        return 1;
    }

    if (!result.sla_at_risk) {
        fprintf(stderr, "[FAIL] SLA risk detection\n");
        return 1;
    }

    if (!result.requires_operator) {
        fprintf(stderr, "[FAIL] operator boundary\n");
        return 1;
    }

    printf("\n[OK] Chrono Assurance Engine self-test passed\n");
    return 0;
}
