#include "service_registry.h"

#include <stdio.h>

int main(void)
{
    ChronoServiceRegistry registry;

    printf("========================================\n");
    printf(" ChronoOS Service Registry Test\n");
    printf("========================================\n");

    if (service_registry_load(
            "../etc/chrono/services.conf",
            &registry) < 0) {

        printf("[FAIL] Could not load service registry\n");
        return 1;
    }

    printf("[OK] Service registry loaded\n");
    printf("[OK] Services: %d\n", registry.count);

    for (int i = 0; i < registry.count; i++) {
        ChronoService *service = &registry.services[i];

        printf(
            "[SERVICE] %s\n"
            "  executable: %s\n"
            "  policy:     %s\n"
            "  max restart:%d\n",
            service->name,
            service->executable,
            service->restart_policy,
            service->max_restarts
        );
    }

    printf("[OK] Registry validation complete\n");

    return 0;
}
