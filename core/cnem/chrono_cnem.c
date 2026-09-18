#include "chrono_cnem.h"

#include <stdio.h>
#include <string.h>

static bool text_valid(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static void copy_text(char *dst, size_t size, const char *src)
{
    if (dst == NULL || size == 0U) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    (void)snprintf(dst, size, "%s", src);
}

static void add_evidence(
    chrono_cnem_incident_t *incident,
    const char *evidence
)
{
    if (incident == NULL ||
        evidence == NULL ||
        incident->evidence_count >= CHRONO_CNEM_MAX_EVIDENCE) {
        return;
    }

    copy_text(
        incident->evidence[incident->evidence_count],
        sizeof(incident->evidence[incident->evidence_count]),
        evidence
    );

    incident->evidence_count++;
}

bool chrono_cnem_validate_observation(
    const chrono_cnem_observation_t *observation
)
{
    if (observation == NULL) {
        return false;
    }

    if (!text_valid(observation->device_id) ||
        !text_valid(observation->site)) {
        return false;
    }

    if (observation->latency_ms < 0.0 ||
        observation->jitter_ms < 0.0 ||
        observation->packet_loss_percent < 0.0 ||
        observation->packet_loss_percent > 100.0) {
        return false;
    }

    return true;
}

bool chrono_cnem_validate_service(
    const chrono_cnem_service_t *service
)
{
    if (service == NULL) {
        return false;
    }

    if (!text_valid(service->service_id) ||
        !text_valid(service->service_type)) {
        return false;
    }

    if (service->sla_target_percent < 0.0 ||
        service->sla_target_percent > 100.0 ||
        service->availability_percent < 0.0 ||
        service->availability_percent > 100.0) {
        return false;
    }

    return true;
}

static void initialize_incident(
    chrono_cnem_incident_t *incident,
    const chrono_cnem_observation_t *observation
)
{
    (void)snprintf(
        incident->incident_id,
        sizeof(incident->incident_id),
        "CHRONO-CNEM-%s",
        observation->device_id
    );

    incident->severity = CHRONO_CNEM_INFO;
    incident->domain = CHRONO_CNEM_DOMAIN_SYSTEM;
    incident->evidence_count = 0U;
    incident->affected_customers = 0U;
    incident->affected_devices = 1U;
    incident->affected_services = 0U;
    incident->sla_at_risk = false;
    incident->operator_approval_required = true;

    copy_text(
        incident->category,
        sizeof(incident->category),
        "OBSERVATION"
    );

    copy_text(
        incident->origin,
        sizeof(incident->origin),
        "No active degradation detected"
    );

    copy_text(
        incident->root_cause,
        sizeof(incident->root_cause),
        "NONE"
    );
}

bool chrono_cnem_correlate(
    const chrono_cnem_observation_t *observation,
    const chrono_cnem_service_t *service,
    chrono_cnem_incident_t *incident
)
{
    if (!chrono_cnem_validate_observation(observation) ||
        !chrono_cnem_validate_service(service) ||
        incident == NULL) {
        return false;
    }

    (void)memset(incident, 0, sizeof(*incident));
    initialize_incident(incident, observation);

    incident->affected_customers = service->affected_customers;
    incident->affected_services = 1U;

    if (service->availability_percent < service->sla_target_percent) {
        incident->sla_at_risk = true;

        add_evidence(
            incident,
            "SLA availability below target"
        );
    }

    /*
     * Correlation priority:
     * connectivity -> routing -> DNS -> PON -> radio -> performance.
     */

    if (!observation->gateway_ok ||
        !observation->ipv4_ok ||
        !observation->interface_name[0]) {

        incident->severity = CHRONO_CNEM_CRITICAL;
        incident->domain = CHRONO_CNEM_DOMAIN_NETWORK;

        copy_text(
            incident->root_cause,
            sizeof(incident->root_cause),
            "CONNECTIVITY"
        );

        copy_text(
            incident->category,
            sizeof(incident->category),
            "NETWORK"
        );

        copy_text(
            incident->origin,
            sizeof(incident->origin),
            "Primary connectivity path degraded"
        );

        add_evidence(
            incident,
            "gateway/IPv4/interface connectivity failure"
        );

        return true;
    }

    if (!observation->routing_ok || !observation->bgp_ok) {
        incident->severity = CHRONO_CNEM_CRITICAL;
        incident->domain = CHRONO_CNEM_DOMAIN_ROUTING;

        copy_text(
            incident->root_cause,
            sizeof(incident->root_cause),
            "ROUTING"
        );

        copy_text(
            incident->category,
            sizeof(incident->category),
            "ROUTING"
        );

        copy_text(
            incident->origin,
            sizeof(incident->origin),
            "Routing control plane degraded"
        );

        add_evidence(
            incident,
            "routing/BGP degradation"
        );

        if (!observation->rpki_ok) {
            incident->severity = CHRONO_CNEM_CRITICAL;

            add_evidence(
                incident,
                "RPKI validation problem"
            );
        }

        return true;
    }

    if (!observation->dns_ok) {
        incident->severity = CHRONO_CNEM_WARNING;
        incident->domain = CHRONO_CNEM_DOMAIN_NETWORK;

        copy_text(
            incident->root_cause,
            sizeof(incident->root_cause),
            "DNS"
        );

        copy_text(
            incident->category,
            sizeof(incident->category),
            "SERVICE"
        );

        copy_text(
            incident->origin,
            sizeof(incident->origin),
            "DNS resolution path degraded"
        );

        add_evidence(
            incident,
            "DNS health check failed"
        );

        return true;
    }

    if (!observation->pon_ok) {
        incident->severity = CHRONO_CNEM_WARNING;
        incident->domain = CHRONO_CNEM_DOMAIN_PON;

        copy_text(
            incident->root_cause,
            sizeof(incident->root_cause),
            "PON"
        );

        copy_text(
            incident->category,
            sizeof(incident->category),
            "ACCESS"
        );

        copy_text(
            incident->origin,
            sizeof(incident->origin),
            "PON/ONU access layer degraded"
        );

        add_evidence(
            incident,
            "PON/ONU health check failed"
        );

        return true;
    }

    if (!observation->radio_ok) {
        incident->severity = CHRONO_CNEM_WARNING;
        incident->domain = CHRONO_CNEM_DOMAIN_RADIO;

        copy_text(
            incident->root_cause,
            sizeof(incident->root_cause),
            "RADIO"
        );

        copy_text(
            incident->category,
            sizeof(incident->category),
            "WIRELESS"
        );

        copy_text(
            incident->origin,
            sizeof(incident->origin),
            "Radio access layer degraded"
        );

        add_evidence(
            incident,
            "radio health check failed"
        );

        return true;
    }

    if (observation->packet_loss_percent >= 5.0 ||
        observation->latency_ms >= 200.0 ||
        observation->jitter_ms >= 50.0) {

        incident->severity = CHRONO_CNEM_WARNING;
        incident->domain = CHRONO_CNEM_DOMAIN_NETWORK;

        copy_text(
            incident->root_cause,
            sizeof(incident->root_cause),
            "PERFORMANCE"
        );

        copy_text(
            incident->category,
            sizeof(incident->category),
            "NETWORK"
        );

        copy_text(
            incident->origin,
            sizeof(incident->origin),
            "Network performance degradation"
        );

        add_evidence(
            incident,
            "latency/jitter/packet-loss threshold exceeded"
        );

        return true;
    }

    add_evidence(
        incident,
        "No degradation threshold exceeded"
    );

    return true;
}

const char *chrono_cnem_severity_name(
    chrono_cnem_severity_t severity
)
{
    switch (severity) {
        case CHRONO_CNEM_INFO:
            return "INFO";
        case CHRONO_CNEM_WARNING:
            return "WARNING";
        case CHRONO_CNEM_CRITICAL:
            return "CRITICAL";
        case CHRONO_CNEM_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

const char *chrono_cnem_domain_name(
    chrono_cnem_domain_t domain
)
{
    switch (domain) {
        case CHRONO_CNEM_DOMAIN_NETWORK:
            return "NETWORK";
        case CHRONO_CNEM_DOMAIN_ROUTING:
            return "ROUTING";
        case CHRONO_CNEM_DOMAIN_RADIO:
            return "RADIO";
        case CHRONO_CNEM_DOMAIN_PON:
            return "PON";
        case CHRONO_CNEM_DOMAIN_SECURITY:
            return "SECURITY";
        case CHRONO_CNEM_DOMAIN_SERVICE:
            return "SERVICE";
        case CHRONO_CNEM_DOMAIN_SYSTEM:
            return "SYSTEM";
        case CHRONO_CNEM_DOMAIN_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

void chrono_cnem_print_incident(
    const chrono_cnem_incident_t *incident
)
{
    if (incident == NULL) {
        return;
    }

    printf("\n=== CHRONO NETWORK EVENT MODEL ===\n");
    printf("Version:             %s\n", CHRONO_CNEM_VERSION);
    printf("Incident:            %s\n", incident->incident_id);
    printf("Severity:            %s\n",
           chrono_cnem_severity_name(incident->severity));
    printf("Domain:              %s\n",
           chrono_cnem_domain_name(incident->domain));

    printf("\n--- ROOT CAUSE ---\n");
    printf("Root cause:          %s\n", incident->root_cause);
    printf("Category:            %s\n", incident->category);
    printf("Origin:              %s\n", incident->origin);

    printf("\n--- IMPACT ---\n");
    printf("Affected customers:  %u\n", incident->affected_customers);
    printf("Affected devices:    %u\n", incident->affected_devices);
    printf("Affected services:   %u\n", incident->affected_services);
    printf("SLA at risk:         %s\n",
           incident->sla_at_risk ? "YES" : "NO");

    printf("\n--- EVIDENCE ---\n");
    printf("Evidence count:      %zu\n", incident->evidence_count);

    for (size_t i = 0U; i < incident->evidence_count; ++i) {
        printf("[%zu] %s\n", i + 1U, incident->evidence[i]);
    }

    printf("\n--- CONTROL ---\n");
    printf("Operator approval:   %s\n",
           incident->operator_approval_required ? "REQUIRED" : "NOT REQUIRED");
}
