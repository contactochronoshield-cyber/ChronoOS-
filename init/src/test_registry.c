#include "service_registry.h"

#include <stdio.h>

int main(void)
{
    ChronoServiceRegistry registry;

    int result = service_registry_load(
        "../etc/chrono/services.conf",
        &registry
    );

    if (result < 0) {
        fprintf(
            stderr,
            "ERROR: unable to load services.conf\n"
        );
        return 1;
    }

    service_registry_print(&registry);

    return 0;
}
