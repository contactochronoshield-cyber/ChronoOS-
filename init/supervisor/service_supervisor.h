#ifndef CHRONO_SERVICE_SUPERVISOR_H
#define CHRONO_SERVICE_SUPERVISOR_H

#include "../include/service_registry.h"

typedef enum {
    CHRONO_SERVICE_STOPPED = 0,
    CHRONO_SERVICE_STARTING,
    CHRONO_SERVICE_RUNNING,
    CHRONO_SERVICE_FAILED
} ChronoServiceState;

typedef struct {
    ChronoService config;
    ChronoServiceState state;
    int restart_count;
    int pid;
} ChronoServiceRuntime;

typedef struct {
    ChronoServiceRuntime services[CHRONO_MAX_SERVICES];
    int count;
} ChronoServiceSupervisor;

int supervisor_init(
    ChronoServiceSupervisor *supervisor,
    const ChronoServiceRegistry *registry
);

int supervisor_start(
    ChronoServiceSupervisor *supervisor,
    int index
);

int supervisor_stop(
    ChronoServiceSupervisor *supervisor,
    int index
);

int supervisor_restart(
    ChronoServiceSupervisor *supervisor,
    int index
);

int supervisor_poll(
    ChronoServiceSupervisor *supervisor
);

int supervisor_handle_failures(
    ChronoServiceSupervisor *supervisor
);

void supervisor_status(
    const ChronoServiceSupervisor *supervisor
);

#endif
