#ifndef CHRONO_SERVICE_REGISTRY_H
#define CHRONO_SERVICE_REGISTRY_H

#define CHRONO_MAX_SERVICES 32
#define CHRONO_NAME_MAX 64
#define CHRONO_PATH_MAX 256
#define CHRONO_POLICY_MAX 32

typedef struct {
    char name[CHRONO_NAME_MAX];
    char executable[CHRONO_PATH_MAX];
    char restart_policy[CHRONO_POLICY_MAX];
    int max_restarts;
} ChronoService;

typedef struct {
    ChronoService services[CHRONO_MAX_SERVICES];
    int count;
} ChronoServiceRegistry;

int service_registry_load(
    const char *path,
    ChronoServiceRegistry *registry
);

void service_registry_print(
    const ChronoServiceRegistry *registry
);

#endif
