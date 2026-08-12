#include "service_supervisor.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    ChronoServiceRegistry registry;
    ChronoServiceSupervisor supervisor;

    printf("========================================\n");
    printf(" ChronoOS Supervisor Restart Test\n");
    printf("========================================\n");

    if (service_registry_load(
            "../etc/chrono/services.conf",
            &registry) < 0)
        return 1;

    if (supervisor_init(&supervisor, &registry) < 0)
        return 1;

    printf("[OK] Registry loaded\n");
    printf("[OK] Supervisor initialized\n");

    for (int attempt = 0; attempt < 5; attempt++) {

        printf("\n[TEST] Monitoring cycle %d\n", attempt + 1);

        if (supervisor.services[1].state ==
            CHRONO_SERVICE_STOPPED) {

            supervisor_start(&supervisor, 1);
        }

        sleep(1);

        supervisor_poll(&supervisor);

        supervisor_handle_failures(&supervisor);

        supervisor_status(&supervisor);
    }

    printf("\n[OK] Restart policy test completed\n");

    return 0;
}
