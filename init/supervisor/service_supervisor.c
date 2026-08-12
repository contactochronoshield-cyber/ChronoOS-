#include "service_supervisor.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static const char *state_name(ChronoServiceState state)
{
    switch (state) {
        case CHRONO_SERVICE_STOPPED:
            return "STOPPED";
        case CHRONO_SERVICE_STARTING:
            return "STARTING";
        case CHRONO_SERVICE_RUNNING:
            return "RUNNING";
        case CHRONO_SERVICE_FAILED:
            return "FAILED";
        default:
            return "UNKNOWN";
    }
}

int supervisor_init(
    ChronoServiceSupervisor *supervisor,
    const ChronoServiceRegistry *registry)
{
    if (!supervisor || !registry)
        return -1;

    memset(supervisor, 0, sizeof(*supervisor));

    supervisor->count = registry->count;

    for (int i = 0; i < registry->count; i++) {
        supervisor->services[i].config = registry->services[i];
        supervisor->services[i].state = CHRONO_SERVICE_STOPPED;
        supervisor->services[i].restart_count = 0;
        supervisor->services[i].pid = -1;
    }

    return 0;
}

int supervisor_start(
    ChronoServiceSupervisor *supervisor,
    int index)
{
    if (!supervisor || index < 0 || index >= supervisor->count)
        return -1;

    ChronoServiceRuntime *service =
        &supervisor->services[index];

    if (service->state == CHRONO_SERVICE_RUNNING)
        return 0;

    printf(
        "[SUPERVISOR] Starting service: %s\n",
        service->config.name
    );

    service->state = CHRONO_SERVICE_STARTING;

    pid_t pid = fork();

    if (pid < 0) {
        perror("[SUPERVISOR] fork");
        service->state = CHRONO_SERVICE_FAILED;
        return -1;
    }

    if (pid == 0) {
        execl(
            service->config.executable,
            service->config.executable,
            (char *)NULL
        );

        perror("[SUPERVISOR] exec");
        _exit(127);
    }

    service->pid = (int)pid;
    service->state = CHRONO_SERVICE_RUNNING;

    printf(
        "[SUPERVISOR] %s RUNNING (PID=%d)\n",
        service->config.name,
        service->pid
    );

    return 0;
}

int supervisor_poll(
    ChronoServiceSupervisor *supervisor)
{
    if (!supervisor)
        return -1;

    for (int i = 0; i < supervisor->count; i++) {

        ChronoServiceRuntime *service =
            &supervisor->services[i];

        if (service->pid <= 0)
            continue;

        if (service->state != CHRONO_SERVICE_RUNNING)
            continue;

        int status = 0;

        pid_t result = waitpid(
            (pid_t)service->pid,
            &status,
            WNOHANG
        );

        if (result == 0)
            continue;

        if (result < 0) {
            perror("[SUPERVISOR] waitpid");
            service->state = CHRONO_SERVICE_FAILED;
            service->pid = -1;
            continue;
        }

        service->pid = -1;

        if (WIFEXITED(status)) {

            int exit_code = WEXITSTATUS(status);

            printf(
                "[SUPERVISOR] %s exited with code %d\n",
                service->config.name,
                exit_code
            );

        } else if (WIFSIGNALED(status)) {

            printf(
                "[SUPERVISOR] %s terminated by signal %d\n",
                service->config.name,
                WTERMSIG(status)
            );
        }

        service->state = CHRONO_SERVICE_FAILED;
    }

    return 0;
}

int supervisor_stop(
    ChronoServiceSupervisor *supervisor,
    int index)
{
    if (!supervisor || index < 0 || index >= supervisor->count)
        return -1;

    ChronoServiceRuntime *service =
        &supervisor->services[index];

    if (service->pid > 0) {

        printf(
            "[SUPERVISOR] Stopping %s (PID %d)\n",
            service->config.name,
            service->pid
        );

        kill((pid_t)service->pid, SIGTERM);

        waitpid(
            (pid_t)service->pid,
            NULL,
            0
        );
    }

    service->pid = -1;
    service->state = CHRONO_SERVICE_STOPPED;

    return 0;
}

int supervisor_restart(
    ChronoServiceSupervisor *supervisor,
    int index)
{
    if (!supervisor || index < 0 || index >= supervisor->count)
        return -1;

    ChronoServiceRuntime *service =
        &supervisor->services[index];

    if (service->restart_count >= service->config.max_restarts) {

        printf(
            "[SUPERVISOR] Restart limit exceeded: %s\n",
            service->config.name
        );

        service->state = CHRONO_SERVICE_FAILED;

        return -1;
    }

    service->restart_count++;

    printf(
        "[SUPERVISOR] Restarting %s (%d/%d)\n",
        service->config.name,
        service->restart_count,
        service->config.max_restarts
    );

    service->state = CHRONO_SERVICE_STOPPED;

    return supervisor_start(supervisor, index);
}

void supervisor_status(
    const ChronoServiceSupervisor *supervisor)
{
    if (!supervisor)
        return;

    printf("\n=== ChronoOS Service Supervisor ===\n");

    for (int i = 0; i < supervisor->count; i++) {

        const ChronoServiceRuntime *service =
            &supervisor->services[i];

        printf(
            "%-16s %-10s PID=%d restarts=%d/%d\n",
            service->config.name,
            state_name(service->state),
            service->pid,
            service->restart_count,
            service->config.max_restarts
        );
    }

    printf("====================================\n\n");
}

int supervisor_handle_failures(
    ChronoServiceSupervisor *supervisor)
{
    if (!supervisor)
        return -1;

    for (int i = 0; i < supervisor->count; i++) {

        ChronoServiceRuntime *service =
            &supervisor->services[i];

        if (service->state != CHRONO_SERVICE_FAILED)
            continue;

        if (strcmp(
                service->config.restart_policy,
                "on-failure") != 0) {

            continue;
        }

        if (service->restart_count >=
            service->config.max_restarts) {

            printf(
                "[SUPERVISOR] Restart limit exceeded: %s\n",
                service->config.name
            );

            /*
             * Disable further restart attempts for this
             * failed service. The service remains FAILED.
             */
            strncpy(
                service->config.restart_policy,
                "none",
                sizeof(service->config.restart_policy) - 1
            );

            service->config.restart_policy[
                sizeof(service->config.restart_policy) - 1
            ] = '\0';

            continue;
        }

        printf(
            "[SUPERVISOR] Failure detected: %s\n",
            service->config.name
        );

        if (supervisor_restart(supervisor, i) < 0) {
            printf(
                "[SUPERVISOR] Restart failed: %s\n",
                service->config.name
            );
        }
    }

    return 0;
}
