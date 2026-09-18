#include "chrono_assurance.h"

#include <stdio.h>
#include <string.h>

static void append_evidence(
    chrono_assurance_result_t *result,
    const char *text
) {
    size_t used;

    if (result == NULL || text == NULL) {
        return;
    }

    used = strlen(result->evidence);

    if (used >= sizeof(result->evidence) - 1U) {
        return;
    }

    if (used > 0U) {
        snprintf(
            result->evidence + used,
            sizeof(result->evidence) - used,
            "; %s",
            text
        );
    } else {
        snprintf(
            result->evidence,
            sizeof(result->evidence),
            "%s",
            text
        );
    }

    result->evidence_count++;
}

int chrono_assurance_validate(
    const chrono_assurance_observation_t *observation
) {
    if (observation == NULL) {
        return 0;
    }

    if (observation->device_id[0] == '\0') {
        return 0;
    }

    if (observation->affected_customers > 1000000000U) {
        return 0;
    }

    if (observation->packet_loss_pct < 0.0 ||
        observation->packet_loss_pct > 100.0) {
        return 0;
    }

    if (observation->latency_ms < 0.0 ||
        observation->jitter_ms < 0.0) {
        return 0;
    }

    return 1;
}

static void set_root(
    chrono_assurance_result_t *result,
    chrono_root_cause_t root,
    const char *category,
    const char *origin
) {
    result->root_cause = root;

    snprintf(
        result->category,
        sizeof(result->category),
        "%s",
        category
    );

    snprintf(
        result->origin,
        sizeof(result->origin),
        "%s",
        origin
    );
}

int chrono_assurance_analyze(
    const chrono_assurance_observation_t *observation,
    chrono_assurance_result_t *result
) {
    unsigned int domains = 0U;

    if (!chrono_assurance_validate(observation) || result == NULL) {
        return 0;
    }

    memset(result, 0, sizeof(*result));

    result->status = CHRONO_ASSURANCE_OK;
    result->requires_operator = 1;
    result->rollback_available = 1;

    snprintf(
        result->incident_id,
        sizeof(result->incident_id),
        "ASSURANCE-%s",
        observation->device_id
    );

    result->affected_customers = observation->affected_customers;
    result->affected_services = observation->affected_services;

    if (!observation->management_ok) {
        domains++;
        append_evidence(result, "management unreachable");
    }

    if (!observation->interface_ok) {
        domains++;
        append_evidence(result, "interface unavailable");
    }

    if (!observation->gateway_ok) {
        domains++;
        append_evidence(result, "gateway failure");
    }

    if (!observation->routing_ok || !observation->bgp_ok) {
        domains++;
        append_evidence(result, "routing/BGP degradation");
    }

    if (!observation->rpki_ok) {
        domains++;
        append_evidence(result, "RPKI validation problem");
    }

    if (!observation->dns_ok) {
        domains++;
        append_evidence(result, "DNS failure");
    }

    if (!observation->ipv4_ok || !observation->ipv6_ok) {
        domains++;
        append_evidence(result, "IP connectivity degradation");
    }

    if (!observation->pon_ok) {
        domains++;
        append_evidence(result, "PON/ONU degradation");
    }

    if (!observation->radio_ok) {
        domains++;
        append_evidence(result, "radio/antenna degradation");
    }

    if (!observation->certificate_ok) {
        domains++;
        append_evidence(result, "certificate validation problem");
    }

    if (observation->configuration_changed) {
        domains++;
        append_evidence(result, "recent configuration change");
    }

    if (observation->packet_loss_pct >= 5.0 ||
        observation->latency_ms >= 200.0 ||
        observation->jitter_ms >= 50.0) {
        domains++;
        append_evidence(result, "performance degradation");
    }

    if (observation->sla_target_pct > 0.0 &&
        observation->current_availability_pct <
        observation->sla_target_pct) {
        result->sla_at_risk = 1;
        append_evidence(result, "SLA target below threshold");
    }

    if (domains == 0U) {
        snprintf(
            result->category,
            sizeof(result->category),
            "NONE"
        );

        snprintf(
            result->origin,
            sizeof(result->origin),
            "No correlated anomaly detected"
        );

        snprintf(
            result->evidence,
            sizeof(result->evidence),
            "All assurance domains passed verification."
        );

        snprintf(
            result->recommendation,
            sizeof(result->recommendation),
            "Continue monitoring and preserve evidence."
        );

        return 1;
    }

    result->status = CHRONO_ASSURANCE_DEGRADED;

    if (!observation->interface_ok ||
        !observation->gateway_ok ||
        !observation->management_ok) {
        set_root(
            result,
            CHRONO_ROOT_LINK,
            "CONNECTIVITY",
            "Primary connectivity path degraded"
        );
    } else if (!observation->routing_ok || !observation->bgp_ok) {
        set_root(
            result,
            CHRONO_ROOT_ROUTING,
            "ROUTING",
            "Routing control plane degraded"
        );
    } else if (!observation->dns_ok) {
        set_root(
            result,
            CHRONO_ROOT_DNS,
            "DNS",
            "Name resolution path degraded"
        );
    } else if (!observation->pon_ok) {
        set_root(
            result,
            CHRONO_ROOT_PON,
            "ACCESS",
            "PON/ONU access domain degraded"
        );
    } else if (!observation->radio_ok) {
        set_root(
            result,
            CHRONO_ROOT_RADIO,
            "RADIO",
            "Radio/antenna domain degraded"
        );
    } else if (!observation->certificate_ok) {
        set_root(
            result,
            CHRONO_ROOT_CERTIFICATE,
            "SECURITY",
            "Certificate trust validation degraded"
        );
    } else if (observation->configuration_changed) {
        set_root(
            result,
            CHRONO_ROOT_CONFIGURATION,
            "CONFIGURATION",
            "Recent configuration change correlates with degradation"
        );
    } else {
        set_root(
            result,
            CHRONO_ROOT_MULTI_DOMAIN,
            "MULTI_DOMAIN",
            "Multiple network domains show correlated degradation"
        );
    }

    if (result->sla_at_risk) {
        snprintf(
            result->recommendation,
            sizeof(result->recommendation),
            "Freeze non-essential changes, preserve evidence, simulate remediation, require operator approval, then verify and retain rollback state."
        );
    } else {
        snprintf(
            result->recommendation,
            sizeof(result->recommendation),
            "Correlate affected domains, simulate the proposed remediation, require operator approval, execute only authorized changes, then verify the result."
        );
    }

    return 1;
}

const char *chrono_assurance_status_name(
    chrono_assurance_status_t status
) {
    switch (status) {
        case CHRONO_ASSURANCE_OK:
            return "OK";
        case CHRONO_ASSURANCE_DEGRADED:
            return "DEGRADED";
        case CHRONO_ASSURANCE_CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

const char *chrono_assurance_root_name(
    chrono_root_cause_t root
) {
    switch (root) {
        case CHRONO_ROOT_LINK:
            return "LINK";
        case CHRONO_ROOT_ROUTING:
            return "ROUTING";
        case CHRONO_ROOT_DNS:
            return "DNS";
        case CHRONO_ROOT_AUTH:
            return "AUTH";
        case CHRONO_ROOT_PON:
            return "PON";
        case CHRONO_ROOT_RADIO:
            return "RADIO";
        case CHRONO_ROOT_CAPACITY:
            return "CAPACITY";
        case CHRONO_ROOT_CONFIGURATION:
            return "CONFIGURATION";
        case CHRONO_ROOT_CERTIFICATE:
            return "CERTIFICATE";
        case CHRONO_ROOT_MULTI_DOMAIN:
            return "MULTI_DOMAIN";
        default:
            return "NONE";
    }
}

void chrono_assurance_print_report(
    const chrono_assurance_observation_t *observation,
    const chrono_assurance_result_t *result
) {
    if (observation == NULL || result == NULL) {
        return;
    }

    printf("\n=== CHRONO ASSURANCE ENGINE ===\n");
    printf("Version:             %s\n", CHRONO_ASSURANCE_VERSION);
    printf("Device:              %s\n", observation->device_id);
    printf("Site:                %s\n", observation->site_id);
    printf("Service:             %s\n", observation->service_id);

    printf("\n--- ROOT CAUSE ---\n");
    printf("Status:              %s\n",
           chrono_assurance_status_name(result->status));
    printf("Root cause:          %s\n",
           chrono_assurance_root_name(result->root_cause));
    printf("Category:            %s\n", result->category);
    printf("Origin:              %s\n", result->origin);

    printf("\n--- IMPACT ---\n");
    printf("Affected customers:  %u\n",
           result->affected_customers);
    printf("Affected services:   %u\n",
           result->affected_services);
    printf("SLA at risk:         %s\n",
           result->sla_at_risk ? "YES" : "NO");

    printf("\n--- EVIDENCE ---\n");
    printf("Evidence count:      %u\n",
           result->evidence_count);
    printf("Evidence:            %s\n",
           result->evidence);

    printf("\n--- RECOMMENDATION ---\n");
    printf("%s\n", result->recommendation);

    printf("\n--- CONTROL BOUNDARY ---\n");
    printf("Operator approval:   %s\n",
           result->requires_operator ? "REQUIRED" : "NOT REQUIRED");
    printf("Rollback state:      %s\n",
           result->rollback_available ? "AVAILABLE" : "UNAVAILABLE");
    printf("External access:     NOT performed\n");
    printf("Configuration change:NOT performed\n");

    printf("\nSTATUS: %s\n",
           chrono_assurance_status_name(result->status));
}
