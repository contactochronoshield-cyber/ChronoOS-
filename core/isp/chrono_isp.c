#include "chrono_isp.h"

#include <stdio.h>
#include <string.h>

int chrono_isp_validate(const chrono_isp_telemetry_t *t)
{
    if (!t)
        return 0;

    if (!t->node_id[0])
        return 0;

    if (t->latency_ms < 0 ||
        t->jitter_ms < 0 ||
        t->packet_loss_pct < 0 ||
        t->packet_loss_pct > 100 ||
        t->cpu_pct < 0 ||
        t->cpu_pct > 100 ||
        t->memory_pct < 0 ||
        t->memory_pct > 100 ||
        t->utilization_pct < 0 ||
        t->utilization_pct > 100 ||
        t->affected_customers < 0)
        return 0;

    return 1;
}

int chrono_isp_analyze(
    const chrono_isp_telemetry_t *t,
    chrono_isp_incident_t *incident)
{
    if (!chrono_isp_validate(t) || !incident)
        return -1;

    memset(incident, 0, sizeof(*incident));

    snprintf(incident->incident_id,
             sizeof(incident->incident_id),
             "CHRONO-ISP-%s",
             t->node_id);

    snprintf(incident->node_id,
             sizeof(incident->node_id),
             "%s",
             t->node_id);

    incident->affected_customers = t->affected_customers;

    if (t->rpki_invalid) {
        incident->severity = CHRONO_SEV_CRITICAL;

        snprintf(incident->category,
                 sizeof(incident->category),
                 "BGP/RPKI");

        snprintf(incident->probable_origin,
                 sizeof(incident->probable_origin),
                 "Ruta RPKI INVALID detectada");

        snprintf(incident->evidence,
                 sizeof(incident->evidence),
                 "rpki_invalid=1; bgp_up=%d",
                 t->bgp_up);

        snprintf(incident->recommendation,
                 sizeof(incident->recommendation),
                 "Validar ROA, origen del prefijo y política ROV.");

        return 1;
    }

    if (!t->bgp_up) {
        incident->severity = CHRONO_SEV_CRITICAL;

        snprintf(incident->category,
                 sizeof(incident->category),
                 "BGP");

        snprintf(incident->probable_origin,
                 sizeof(incident->probable_origin),
                 "Sesión BGP caída");

        snprintf(incident->evidence,
                 sizeof(incident->evidence),
                 "bgp_up=0; loss=%.2f%%",
                 t->packet_loss_pct);

        snprintf(incident->recommendation,
                 sizeof(incident->recommendation),
                 "Revisar vecino BGP, transporte y reachability.");

        return 1;
    }

    if (t->pon_los || t->pon_flapping || t->onu_errors >= 10) {
        incident->severity =
            (t->pon_los || t->pon_flapping)
            ? CHRONO_SEV_CRITICAL
            : CHRONO_SEV_WARNING;

        snprintf(incident->category,
                 sizeof(incident->category),
                 "PON");

        snprintf(incident->probable_origin,
                 sizeof(incident->probable_origin),
                 "Problema PON/ONU");

        snprintf(incident->evidence,
                 sizeof(incident->evidence),
                 "LOS=%d; flapping=%d; ONU_errors=%d",
                 t->pon_los,
                 t->pon_flapping,
                 t->onu_errors);

        snprintf(incident->recommendation,
                 sizeof(incident->recommendation),
                 "Revisar niveles ópticos, ONU y eventos de flapping.");

        return 1;
    }

    if (t->certificate_expired ||
        t->management_exposed ||
        t->configuration_changed) {

        incident->severity =
            t->certificate_expired
            ? CHRONO_SEV_CRITICAL
            : CHRONO_SEV_WARNING;

        snprintf(incident->category,
                 sizeof(incident->category),
                 "ISP-SECURITY");

        snprintf(incident->probable_origin,
                 sizeof(incident->probable_origin),
                 "Evento de seguridad operacional");

        snprintf(incident->evidence,
                 sizeof(incident->evidence),
                 "cert=%d; management=%d; config=%d",
                 t->certificate_expired,
                 t->management_exposed,
                 t->configuration_changed);

        snprintf(incident->recommendation,
                 sizeof(incident->recommendation),
                 "Revisar administración, configuración y certificados.");

        return 1;
    }

    if (t->packet_loss_pct >= 2.0 ||
        t->latency_ms >= 100.0 ||
        t->jitter_ms >= 30.0) {

        incident->severity = CHRONO_SEV_WARNING;

        if (t->packet_loss_pct >= 5.0 ||
            t->latency_ms >= 200.0)
            incident->severity = CHRONO_SEV_CRITICAL;

        snprintf(incident->category,
                 sizeof(incident->category),
                 "NETWORK");

        snprintf(incident->probable_origin,
                 sizeof(incident->probable_origin),
                 "Degradación de conectividad");

        snprintf(incident->evidence,
                 sizeof(incident->evidence),
                 "latency=%.2fms; jitter=%.2fms; loss=%.2f%%",
                 t->latency_ms,
                 t->jitter_ms,
                 t->packet_loss_pct);

        snprintf(incident->recommendation,
                 sizeof(incident->recommendation),
                 "Correlacionar enlaces, interfaces, radio y capacidad.");

        return 1;
    }

    if (t->utilization_pct >= 90.0) {
        incident->severity = CHRONO_SEV_CRITICAL;

        snprintf(incident->category,
                 sizeof(incident->category),
                 "CAPACITY");

        snprintf(incident->probable_origin,
                 sizeof(incident->probable_origin),
                 "Capacidad >= 90%%");

        snprintf(incident->evidence,
                 sizeof(incident->evidence),
                 "utilization=%.2f%%",
                 t->utilization_pct);

        snprintf(incident->recommendation,
                 sizeof(incident->recommendation),
                 "Evaluar expansión y redistribución de tráfico.");

        return 1;
    }

    if (t->utilization_pct >= 80.0) {
        incident->severity = CHRONO_SEV_WARNING;

        snprintf(incident->category,
                 sizeof(incident->category),
                 "CAPACITY");

        snprintf(incident->probable_origin,
                 sizeof(incident->probable_origin),
                 "Presión de capacidad");

        snprintf(incident->evidence,
                 sizeof(incident->evidence),
                 "utilization=%.2f%%",
                 t->utilization_pct);

        snprintf(incident->recommendation,
                 sizeof(incident->recommendation),
                 "Registrar tendencia y evaluar crecimiento.");

        return 1;
    }

    incident->severity = CHRONO_SEV_INFO;

    snprintf(incident->category,
             sizeof(incident->category),
             "HEALTHY");

    snprintf(incident->probable_origin,
             sizeof(incident->probable_origin),
             "Sin incidente detectado");

    snprintf(incident->evidence,
             sizeof(incident->evidence),
             "Telemetría dentro de umbrales.");

    snprintf(incident->recommendation,
             sizeof(incident->recommendation),
             "Continuar observación.");

    return 0;
}

void chrono_isp_print_report(
    const chrono_isp_telemetry_t *t,
    const chrono_isp_incident_t *incident)
{
    const char *severity = "INFO";

    if (incident->severity == CHRONO_SEV_WARNING)
        severity = "WARNING";

    if (incident->severity == CHRONO_SEV_CRITICAL)
        severity = "CRITICAL";

    printf("\n=== CHRONO ISP INTELLIGENCE ===\n");
    printf("Version:       %s\n", CHRONO_ISP_VERSION);
    printf("Node:          %s\n", t->node_id);
    printf("Type:          %s\n", t->node_type);

    printf("\n--- NETWORK ---\n");
    printf("Latency:       %.2f ms\n", t->latency_ms);
    printf("Jitter:        %.2f ms\n", t->jitter_ms);
    printf("Packet loss:   %.2f %%\n", t->packet_loss_pct);

    printf("\n--- CAPACITY ---\n");
    printf("CPU:           %.2f %%\n", t->cpu_pct);
    printf("Memory:        %.2f %%\n", t->memory_pct);
    printf("Utilization:   %.2f %%\n", t->utilization_pct);

    printf("\n--- BGP / RPKI ---\n");
    printf("BGP:           %s\n", t->bgp_up ? "UP" : "DOWN");
    printf("RPKI valid:    %s\n", t->rpki_valid ? "YES" : "NO");
    printf("RPKI invalid:  %s\n", t->rpki_invalid ? "YES" : "NO");

    printf("\n--- IPv6 ---\n");
    printf("IPv6:          %s\n",
           t->ipv6_enabled ? "ENABLED" : "NOT READY");

    printf("\n--- PON ---\n");
    printf("LOS:           %s\n", t->pon_los ? "YES" : "NO");
    printf("Flapping:      %s\n", t->pon_flapping ? "YES" : "NO");
    printf("ONU errors:    %d\n", t->onu_errors);

    printf("\n--- SECURITY ---\n");
    printf("Management:    %s\n",
           t->management_exposed ? "EXPOSED" : "NOT EXPOSED");
    printf("Config change: %s\n",
           t->configuration_changed ? "YES" : "NO");
    printf("Certificate:   %s\n",
           t->certificate_expired ? "EXPIRED" : "OK");

    printf("\n--- INCIDENT ENGINE ---\n");
    printf("ID:            %s\n", incident->incident_id);
    printf("Severity:      %s\n", severity);
    printf("Category:      %s\n", incident->category);
    printf("Origin:        %s\n", incident->probable_origin);
    printf("Evidence:      %s\n", incident->evidence);
    printf("Recommendation:%s\n", incident->recommendation);
    printf("Impact:        %d customers\n", incident->affected_customers);

    printf("\n--- AUTHORIZATION BOUNDARY ---\n");
    printf("External access:       NOT performed\n");
    printf("Configuration changes: NOT performed\n");
    printf("Operator approval:     REQUIRED\n");

    printf("\nSTATUS: %s\n",
           incident->severity == CHRONO_SEV_CRITICAL
               ? "INCIDENT"
               : incident->severity == CHRONO_SEV_WARNING
                   ? "DEGRADED"
                   : "OPERATIONAL");
}

int chrono_isp_self_test(void)
{
    chrono_isp_telemetry_t t = {0};
    chrono_isp_incident_t incident = {0};

    snprintf(t.node_id, sizeof(t.node_id), "ISP-POP-01");
    snprintf(t.node_type, sizeof(t.node_type), "EDGE");

    t.latency_ms = 185.0;
    t.jitter_ms = 42.0;
    t.packet_loss_pct = 3.5;

    t.cpu_pct = 61.0;
    t.memory_pct = 58.0;
    t.utilization_pct = 84.0;

    t.bgp_up = 1;
    t.rpki_valid = 1;
    t.ipv6_enabled = 1;

    t.onu_errors = 2;
    t.affected_customers = 23;

    if (!chrono_isp_validate(&t))
        return 1;

    if (chrono_isp_analyze(&t, &incident) < 0)
        return 1;

    chrono_isp_print_report(&t, &incident);

    if (incident.severity != CHRONO_SEV_WARNING)
        return 1;

    if (strcmp(incident.category, "NETWORK") != 0)
        return 1;

    printf("\n[OK] Chrono ISP Intelligence self-test passed\n");

    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--self-test") == 0)
        return chrono_isp_self_test();

    printf("Chrono ISP Intelligence %s\n", CHRONO_ISP_VERSION);
    printf("Usage: %s --self-test\n", argv[0]);

    return 0;
}
