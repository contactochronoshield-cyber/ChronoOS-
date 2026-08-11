#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define CHRONO_VERSION "1.1.0-alpha"
#define SERVICE_NAME "chronod"

static void usage(void)
{
    printf(
        "ChronoCTL %s\n\n"
        "Usage:\n"
        "  chronoctl status\n"
        "  chronoctl services\n"
        "  chronoctl stop chronod\n"
        "  chronoctl restart chronod\n"
        "  chronoctl logs\n"
        "  chronoctl version\n",
        CHRONO_VERSION
    );
}

static int find_process(const char *name, pid_t *pid_out)
{
    DIR *dir = opendir("/proc");

    if (!dir)
        return -1;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        char *end;
        long pid = strtol(entry->d_name, &end, 10);

        if (*end != '\0' || pid <= 0)
            continue;

        char path[256];
        snprintf(
            path,
            sizeof(path),
            "/proc/%ld/comm",
            pid
        );

        FILE *file = fopen(path, "r");

        if (!file)
            continue;

        char process_name[128] = {0};

        if (fgets(process_name, sizeof(process_name), file)) {

            process_name[strcspn(process_name, "\n")] = '\0';

            if (strcmp(process_name, name) == 0) {
                fclose(file);
                closedir(dir);

                *pid_out = (pid_t)pid;
                return 0;
            }
        }

        fclose(file);
    }

    closedir(dir);
    return -1;
}

static int command_status(void)
{
    pid_t pid;

    printf("ChronoOS %s\n", CHRONO_VERSION);
    printf("--------------------------------\n");

    if (find_process(SERVICE_NAME, &pid) == 0) {

        printf("Init:     RUNNING\n");
        printf("ChronoD:  RUNNING\n");
        printf("PID:      %d\n", pid);

    } else {

        printf("Init:     UNKNOWN\n");
        printf("ChronoD:  STOPPED\n");
    }

    return 0;
}

static int command_services(void)
{
    pid_t pid;

    printf("ChronoOS Services\n");
    printf("--------------------------------\n");

    if (find_process(SERVICE_NAME, &pid) == 0) {
        printf(
            "%-12s RUNNING  PID=%d\n",
            SERVICE_NAME,
            pid
        );
    } else {
        printf(
            "%-12s STOPPED\n",
            SERVICE_NAME
        );
    }

    return 0;
}

static int command_stop(const char *service)
{
    pid_t pid;

    if (strcmp(service, SERVICE_NAME) != 0) {
        fprintf(stderr, "Unknown service: %s\n", service);
        return 1;
    }

    if (find_process(service, &pid) != 0) {
        printf("ChronoD is not running.\n");
        return 0;
    }

    printf(
        "[chronoctl] Stopping %s (PID %d)...\n",
        service,
        pid
    );

    if (kill(pid, SIGTERM) < 0) {
        perror("[chronoctl] kill");
        return 1;
    }

    printf("[chronoctl] Stop signal sent.\n");

    return 0;
}

static int command_restart(const char *service)
{
    pid_t pid;

    if (strcmp(service, SERVICE_NAME) != 0) {
        fprintf(stderr, "Unknown service: %s\n", service);
        return 1;
    }

    if (find_process(service, &pid) == 0) {

        printf(
            "[chronoctl] Stopping %s (PID %d)...\n",
            service,
            pid
        );

        if (kill(pid, SIGTERM) < 0) {
            perror("[chronoctl] kill");
            return 1;
        }

        sleep(1);
    }

    printf(
        "[chronoctl] Service restart requested.\n"
    );

    printf(
        "[chronoctl] The init supervisor will restart %s.\n",
        service
    );

    return 0;
}

static int command_logs(void)
{
    printf("ChronoOS logging interface\n");
    printf("--------------------------------\n");
    printf("Runtime logs are currently emitted by chrono-init and ChronoD.\n");
    printf("Persistent structured logging is scheduled for the next subsystem stage.\n");

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return 1;
    }

    if (strcmp(argv[1], "status") == 0)
        return command_status();

    if (strcmp(argv[1], "services") == 0)
        return command_services();

    if (strcmp(argv[1], "logs") == 0)
        return command_logs();

    if (strcmp(argv[1], "version") == 0) {
        printf("ChronoCTL %s\n", CHRONO_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "stop") == 0) {

        if (argc < 3) {
            fprintf(stderr, "Usage: chronoctl stop chronod\n");
            return 1;
        }

        return command_stop(argv[2]);
    }

    if (strcmp(argv[1], "restart") == 0) {

        if (argc < 3) {
            fprintf(stderr, "Usage: chronoctl restart chronod\n");
            return 1;
        }

        return command_restart(argv[2]);
    }

    fprintf(stderr, "Unknown command: %s\n\n", argv[1]);
    usage();

    return 1;
}
