#define _POSIX_C_SOURCE 200809L

#include "logger.h"
#include "service_registry.h"
#include "service_supervisor.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>

#define CHRONO_VERSION "1.1.0"

static volatile sig_atomic_t shutdown_requested = 0;

static void signal_handler(int sig)
{
    (void)sig;
    shutdown_requested = 1;
}

static int install_signal_handlers(void)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = signal_handler;

    if (sigaction(SIGINT, &sa, NULL) < 0)
        return -1;

    if (sigaction(SIGTERM, &sa, NULL) < 0)
        return -1;

    return 0;
}

int main(void)
{
    ChronoServiceRegistry registry;
    ChronoServiceSupervisor supervisor;

    printf("\n");
    printf("==============================================\n");
    printf("        ChronoOS Init %s\n", CHRONO_VERSION);
    printf("==============================================\n");
    printf("\n");

    printf("[INIT] Development mode: PID %d\n", getpid());

    if (install_signal_handlers() < 0) {
        perror("[INIT] signal setup");
        return EXIT_FAILURE;
    }

    chrono_log("INIT", "INIT", "Initializing runtime");

    int service_count = service_registry_load(
        "../etc/chrono/services.conf",
        &registry
    );

    if (service_count < 0) {
        chrono_log(
            "ERROR",
            "INIT",
            "Unable to load service registry"
        );
        return EXIT_FAILURE;
    }

    chrono_log(
        "OK",
        "INIT",
        "Service registry loaded"
    );

    for (int i = 0; i < registry.count; i++) {
        printf(
            "[SERVICE] %s | %s | %s | max=%d\n",
            registry.services[i].name,
            registry.services[i].executable,
            registry.services[i].restart_policy,
            registry.services[i].max_restarts
        );
    }

    if (supervisor_init(&supervisor, &registry) < 0) {
        chrono_log(
            "ERROR",
            "SUPERVISOR",
            "Unable to initialize service supervisor"
        );
        return EXIT_FAILURE;
    }

    chrono_log(
        "OK",
        "SUPERVISOR",
        "Service supervisor initialized"
    );

    /*
     * Start every service registered in services.conf.
     */
    for (int i = 0; i < registry.count; i++) {

        if (supervisor_start(&supervisor, i) < 0) {

            printf(
                "[ERROR] Unable to start service: %s\n",
                registry.services[i].name
            );

            return EXIT_FAILURE;
        }
    }

    supervisor_status(&supervisor);

    chrono_log(
        "OK",
        "INIT",
        "ChronoOS service supervisor running"
    );

    /*
     * Main supervision loop.
     */
    while (!shutdown_requested) {

        if (supervisor_poll(&supervisor) < 0) {
            chrono_log(
                "ERROR",
                "SUPERVISOR",
                "Service polling failed"
            );
            break;
        }

        if (supervisor_handle_failures(&supervisor) < 0) {
            chrono_log(
                "ERROR",
                "SUPERVISOR",
                "Failure handling failed"
            );
            break;
        }

        sleep(1);
    }

    chrono_log(
        "INIT",
        "INIT",
        "Stopping ChronoOS services"
    );

    /*
     * Stop all running services cleanly.
     */
    for (int i = 0; i < registry.count; i++) {

        if (supervisor.services[i].pid > 0) {

            supervisor_stop(
                &supervisor,
                i
            );
        }
    }

    supervisor_status(&supervisor);

    chrono_log(
        "OK",
        "INIT",
        "ChronoOS shutdown complete"
    );

    return EXIT_SUCCESS;
}
