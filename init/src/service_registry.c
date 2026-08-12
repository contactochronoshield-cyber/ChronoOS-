#include "service_registry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *trim(char *text)
{
    char *end;

    while (*text && isspace((unsigned char)*text))
        text++;

    if (*text == '\0')
        return text;

    end = text + strlen(text) - 1;

    while (end > text && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';

    return text;
}

int service_registry_load(
    const char *path,
    ChronoServiceRegistry *registry)
{
    FILE *file;
    char line[512];

    if (!path || !registry)
        return -1;

    memset(registry, 0, sizeof(*registry));

    file = fopen(path, "r");
    if (!file)
        return -1;

    while (fgets(line, sizeof(line), file)) {
        char *name;
        char *executable;
        char *policy;
        char *max_restarts;
        char *saveptr = NULL;
        ChronoService *service;

        char *clean = trim(line);

        if (*clean == '\0' || *clean == '#')
            continue;

        name = strtok_r(clean, "|", &saveptr);
        executable = strtok_r(NULL, "|", &saveptr);
        policy = strtok_r(NULL, "|", &saveptr);
        max_restarts = strtok_r(NULL, "|", &saveptr);

        if (!name || !executable || !policy || !max_restarts)
            continue;

        name = trim(name);
        executable = trim(executable);
        policy = trim(policy);
        max_restarts = trim(max_restarts);

        if (registry->count >= CHRONO_MAX_SERVICES)
            break;

        if (strlen(name) >= CHRONO_NAME_MAX ||
            strlen(executable) >= CHRONO_PATH_MAX ||
            strlen(policy) >= CHRONO_POLICY_MAX)
            continue;

        service = &registry->services[registry->count];

        snprintf(
            service->name,
            sizeof(service->name),
            "%s",
            name
        );

        snprintf(
            service->executable,
            sizeof(service->executable),
            "%s",
            executable
        );

        snprintf(
            service->restart_policy,
            sizeof(service->restart_policy),
            "%s",
            policy
        );

        service->max_restarts = atoi(max_restarts);

        if (service->max_restarts < 0)
            service->max_restarts = 0;

        registry->count++;
    }

    fclose(file);

    return registry->count;
}

void service_registry_print(
    const ChronoServiceRegistry *registry)
{
    int i;

    if (!registry)
        return;

    printf("Services loaded: %d\n", registry->count);

    for (i = 0; i < registry->count; i++) {
        const ChronoService *service =
            &registry->services[i];

        printf(
            "SERVICE: %s | %s | %s | max=%d\n",
            service->name,
            service->executable,
            service->restart_policy,
            service->max_restarts
        );
    }
}
