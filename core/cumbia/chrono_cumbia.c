#include "chrono_cumbia.h"

#include <stdio.h>
#include <string.h>

static void copy_text(char *dst, size_t dst_size, const char *src)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    snprintf(dst, dst_size, "%s", src);
}

int chrono_cumbia_validate(const chrono_cumbia_device_t *device)
{
    if (device == NULL) {
        return 0;
    }

    if (device->device_id[0] == '\0') {
        return 0;
    }

    if (device->device_type[0] == '\0') {
        return 0;
    }

    if (device->latency_ms < 0.0 ||
        device->jitter_ms < 0.0 ||
        device->packet_loss_pct < 0.0 ||
        device->packet_loss_pct > 100.0) {
        return 0;
    }

    return 1;
}

int chrono_cumbia_authorized_action(
    const chrono_cumbia_device_t *device,
    chrono_cumbia_action_t action
)
{
    if (device == NULL || !device->authorized) {
        return 0;
    }

    /*
     * Cumbia v1 permite observación, conexión, lectura y análisis
     * sobre infraestructura explícitamente autorizada.
     *
     * Las acciones mutativas permanecen bloqueadas hasta una política
     * de autorización explícita de Chrono Control.
     */
    switch (action) {
    case CUMBIA_ACTION_OBSERVE:
    case CUMBIA_ACTION_CONNECT:
    case CUMBIA_ACTION_READ:
    case CUMBIA_ACTION_ANALYZE:
        return 1;

    case CUMBIA_ACTION_CHANGE:
    case CUMBIA_ACTION_RESTART:
    case CUMBIA_ACTION_ISOLATE:
        return 0;

    default:
        return 0;
    }
}

int chrono_cumbia_analyze(
    const chrono_cumbia_device_t *device,
    chrono_cumbia_result_t *result
)
{
    if (!chrono_cumbia_validate(device) || result == NULL) {
        return 0;
    }

    memset(result, 0, sizeof(*result));
    result->status = CUMBIA_STATUS_ONLINE;
    result->requires_operator = 1;

    snprintf(
        result->incident_id,
        sizeof(result->incident_id),
        "CUMBIA-%s",
        device->device_id
    );

    if (!device->authorized) {
        result->status = CUMBIA_STATUS_BLOCKED;
        result->anomaly_count = 1;

        copy_text(result->category, sizeof(result->category), "AUTHORIZATION");
        copy_text(result->origin, sizeof(result->origin),
                  "Dispositivo no autorizado");
        copy_text(result->evidence, sizeof(result->evidence),
                  "Authorization boundary denied operational access.");
        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Registrar y autorizar el dispositivo antes de conectar.");

        return 1;
    }

    if (!device->management_reachable) {
        result->status = CUMBIA_STATUS_BLOCKED;
        result->anomaly_count++;

        copy_text(result->category, sizeof(result->category), "CONNECTIVITY");
        copy_text(result->origin, sizeof(result->origin),
                  "Management endpoint unreachable");
        copy_text(result->evidence, sizeof(result->evidence),
                  "Management channel is unreachable.");

        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Verificar enlace, direccionamiento, gateway y "
                  "credenciales autorizadas.");

        return 1;
    }

    if (!device->ip_reachable || !device->gateway_reachable) {
        result->status = CUMBIA_STATUS_BLOCKED;
        result->anomaly_count++;

        copy_text(result->category, sizeof(result->category), "NETWORK");
        copy_text(result->origin, sizeof(result->origin),
                  "IP or gateway connectivity failure");
        copy_text(result->evidence, sizeof(result->evidence),
                  "Management reachable but IP/gateway verification failed.");
        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Verificar interfaz, direccionamiento, gateway y rutas.");

        return 1;
    }

    if (!device->routing_ok) {
        result->status = CUMBIA_STATUS_DEGRADED;
        result->anomaly_count++;

        copy_text(result->category, sizeof(result->category), "ROUTING");
        copy_text(result->origin, sizeof(result->origin),
                  "Routing verification failed");
        copy_text(result->evidence, sizeof(result->evidence),
                  "IP connectivity exists but routing verification failed.");
        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Correlacionar rutas, interfaces, BGP y RPKI.");
    }

    if (!device->bgp_ok) {
        result->status = CUMBIA_STATUS_DEGRADED;
        result->anomaly_count++;

        copy_text(result->category, sizeof(result->category), "BGP");
        copy_text(result->origin, sizeof(result->origin),
                  "BGP session unavailable");
        copy_text(result->evidence, sizeof(result->evidence),
                  "BGP verification failed.");
        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Verificar sesión BGP, AS-PATH, filtros y RPKI.");
    }

    if (!device->rpki_ok) {
        result->status = CUMBIA_STATUS_DEGRADED;
        result->anomaly_count++;

        copy_text(result->category, sizeof(result->category), "RPKI");
        copy_text(result->origin, sizeof(result->origin),
                  "RPKI verification problem");
        copy_text(result->evidence, sizeof(result->evidence),
                  "RPKI verification did not return an acceptable state.");
        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Revisar ROA, origin AS y estado de validación.");
    }

    if (!device->ipv4_ok || !device->ipv6_ok) {
        result->status = CUMBIA_STATUS_DEGRADED;
        result->anomaly_count++;

        copy_text(result->category, sizeof(result->category), "IPV");
        copy_text(result->origin, sizeof(result->origin),
                  "IP protocol readiness incomplete");
        copy_text(result->evidence, sizeof(result->evidence),
                  "One or more IP protocol checks failed.");
        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Verificar direccionamiento, rutas, DNS y soporte IPv4/IPv6.");
    }

    if (!device->dns_ok) {
        result->status = CUMBIA_STATUS_DEGRADED;
        result->anomaly_count++;

        copy_text(result->category, sizeof(result->category), "DNS");
        copy_text(result->origin, sizeof(result->origin),
                  "DNS verification failed");
        copy_text(result->evidence, sizeof(result->evidence),
                  "Connectivity exists but DNS verification failed.");
        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Verificar resolvers, conectividad DNS y configuración del CPE.");
    }

    if (!device->certificate_ok ||
        device->authentication_failure ||
        device->config_changed) {
        result->status = CUMBIA_STATUS_DEGRADED;
        result->anomaly_count++;

        copy_text(result->category, sizeof(result->category), "SECURITY");

        if (!device->certificate_ok) {
            copy_text(result->origin, sizeof(result->origin),
                      "Certificate verification failure");
            copy_text(result->evidence, sizeof(result->evidence),
                      "Certificate validation failed.");
        } else if (device->authentication_failure) {
            copy_text(result->origin, sizeof(result->origin),
                      "Authentication failure detected");
            copy_text(result->evidence, sizeof(result->evidence),
                      "Authentication failure event detected.");
        } else {
            copy_text(result->origin, sizeof(result->origin),
                      "Configuration change detected");
            copy_text(result->evidence, sizeof(result->evidence),
                      "Configuration state changed.");
        }

        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Correlacionar evento, identidad, configuración y "
                  "telemetría antes de autorizar cambios.");
    }

    if (device->interface_down) {
        result->status = CUMBIA_STATUS_BLOCKED;
        result->anomaly_count++;

        copy_text(result->category, sizeof(result->category), "INTERFACE");
        copy_text(result->origin, sizeof(result->origin),
                  "Interface unavailable");
        copy_text(result->evidence, sizeof(result->evidence),
                  "One or more required interfaces are down.");
        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Verificar estado físico/lógico, errores y configuración.");
    }

    if (device->packet_loss_pct >= 5.0 ||
        device->latency_ms >= 200.0 ||
        device->jitter_ms >= 50.0) {
        result->status = CUMBIA_STATUS_DEGRADED;
        result->anomaly_count++;

        copy_text(result->category, sizeof(result->category), "PERFORMANCE");

        snprintf(
            result->origin,
            sizeof(result->origin),
            "Network performance degradation"
        );

        snprintf(
            result->evidence,
            sizeof(result->evidence),
            "latency=%.2fms; jitter=%.2fms; loss=%.2f%%",
            device->latency_ms,
            device->jitter_ms,
            device->packet_loss_pct
        );

        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Correlacionar enlaces, interfaces, radio, capacidad y rutas.");
    }

    if (result->anomaly_count == 0) {
        copy_text(result->category, sizeof(result->category), "NONE");
        copy_text(result->origin, sizeof(result->origin),
                  "No anomaly detected");
        copy_text(result->evidence, sizeof(result->evidence),
                  "All Cumbia v1 verification checks passed.");
        copy_text(result->recommendation, sizeof(result->recommendation),
                  "Continue monitoring.");
    }

    return 1;
}

static const char *status_name(chrono_cumbia_status_t status)
{
    switch (status) {
    case CUMBIA_STATUS_ONLINE:
        return "ONLINE";
    case CUMBIA_STATUS_DEGRADED:
        return "DEGRADED";
    case CUMBIA_STATUS_BLOCKED:
        return "BLOCKED";
    case CUMBIA_STATUS_OFFLINE:
        return "OFFLINE";
    default:
        return "UNKNOWN";
    }
}

void chrono_cumbia_print_report(
    const chrono_cumbia_device_t *device,
    const chrono_cumbia_result_t *result
)
{
    if (device == NULL || result == NULL) {
        return;
    }

    printf("\n=== CUMBIA ===\n");
    printf("Version:       %s\n", CHRONO_CUMBIA_VERSION);
    printf("Device:        %s\n", device->device_id);
    printf("Type:          %s\n", device->device_type);
    printf("Technology:    %s\n", device->technology);
    printf("Authorized:    %s\n", device->authorized ? "YES" : "NO");

    printf("\n--- CONNECTIVITY ---\n");
    printf("Management:    %s\n",
           device->management_reachable ? "OK" : "FAIL");
    printf("IP:            %s\n",
           device->ip_reachable ? "OK" : "FAIL");
    printf("Gateway:       %s\n",
           device->gateway_reachable ? "OK" : "FAIL");
    printf("DNS:           %s\n",
           device->dns_ok ? "OK" : "FAIL");
    printf("IPv4:          %s\n",
           device->ipv4_ok ? "OK" : "FAIL");
    printf("IPv6:          %s\n",
           device->ipv6_ok ? "OK" : "FAIL");
    printf("Routing:       %s\n",
           device->routing_ok ? "OK" : "FAIL");

    printf("\n--- ROUTING / SECURITY ---\n");
    printf("BGP:           %s\n", device->bgp_ok ? "UP" : "DOWN");
    printf("RPKI:          %s\n", device->rpki_ok ? "VALID" : "CHECK");
    printf("Certificate:   %s\n",
           device->certificate_ok ? "OK" : "FAIL");

    printf("\n--- PERFORMANCE ---\n");
    printf("Latency:       %.2f ms\n", device->latency_ms);
    printf("Jitter:        %.2f ms\n", device->jitter_ms);
    printf("Packet loss:   %.2f %%\n", device->packet_loss_pct);

    printf("\n--- INCIDENT ENGINE ---\n");
    printf("ID:            %s\n", result->incident_id);
    printf("Status:        %s\n", status_name(result->status));
    printf("Anomalies:     %d\n", result->anomaly_count);
    printf("Category:      %s\n", result->category);
    printf("Origin:        %s\n", result->origin);
    printf("Evidence:      %s\n", result->evidence);
    printf("Recommendation:%s\n", result->recommendation);
    printf("Impact:        %d devices\n", device->affected_devices);

    printf("\n--- AUTHORIZATION BOUNDARY ---\n");
    printf("External access:       NOT performed\n");
    printf("Configuration changes: NOT performed\n");
    printf("Operator approval:     REQUIRED\n");

    printf("\nSTATUS: %s\n", status_name(result->status));
}

static int run_self_test(void)
{
    chrono_cumbia_device_t device = {
        .device_id = "POP-03",
        .device_type = "ROUTER",
        .technology = "BGP/IPv4/IPv6",

        .authorized = 1,
        .management_reachable = 1,
        .ip_reachable = 1,
        .gateway_reachable = 1,
        .dns_ok = 1,
        .ipv4_ok = 1,
        .ipv6_ok = 1,
        .routing_ok = 1,
        .bgp_ok = 1,
        .rpki_ok = 1,
        .certificate_ok = 1,

        .latency_ms = 18.0,
        .jitter_ms = 3.0,
        .packet_loss_pct = 0.10,

        .config_changed = 0,
        .authentication_failure = 0,
        .interface_down = 0,

        .affected_devices = 0
    };

    chrono_cumbia_result_t result;

    if (!chrono_cumbia_analyze(&device, &result)) {
        fprintf(stderr, "[FAIL] Cumbia self-test analysis\n");
        return 1;
    }

    chrono_cumbia_print_report(&device, &result);

    if (result.status != CUMBIA_STATUS_ONLINE ||
        result.anomaly_count != 0 ||
        !chrono_cumbia_authorized_action(&device, CUMBIA_ACTION_CONNECT) ||
        !chrono_cumbia_authorized_action(&device, CUMBIA_ACTION_ANALYZE) ||
        chrono_cumbia_authorized_action(&device, CUMBIA_ACTION_CHANGE) ||
        chrono_cumbia_authorized_action(&device, CUMBIA_ACTION_RESTART) ||
        chrono_cumbia_authorized_action(&device, CUMBIA_ACTION_ISOLATE)) {
        fprintf(stderr, "[FAIL] Cumbia authorization/status validation\n");
        return 1;
    }

    printf("\n[OK] Cumbia self-test passed\n");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--self-test") == 0) {
        return run_self_test();
    }

    fprintf(stderr, "Usage: %s --self-test\n", argv[0]);
    return 2;
}
