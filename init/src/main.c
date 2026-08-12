#include "logger.h"
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "service_registry.h"

#define CHRONO_VERSION "1.1.0-alpha"
#define MAX_RESTARTS 3
#define RESTART_DELAY 1

static volatile sig_atomic_t shutdown_requested = 0;
static pid_t chronod_pid = -1;

static void signal_handler(int sig)
{
    (void)sig;
    shutdown_requested = 1;

    if (chronod_pid > 0) {
        kill(chronod_pid, SIGTERM);
    }
}

static void timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm tm_now;

    localtime_r(&now, &tm_now);

    strftime(buffer, size, "%H:%M:%S", &tm_now);
}

static void log_msg(const char *level, const char *message)
{
    char timebuf[16];

    timestamp(timebuf, sizeof(timebuf));

    printf("[%s] [%s] %s\n",
           timebuf,
           level,
           message);

    fflush(stdout);
}

static int install_signal_handlers(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);

    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0)
        return -1;

    if (sigaction(SIGTERM, &sa, NULL) < 0)
        return -1;

    return 0;
}

static int get_project_root(const char *argv0,
                            char *root,
                            size_t root_size)
{
    char executable[PATH_MAX];

    if (realpath(argv0, executable) == NULL) {
        /*
         * argv[0] may be relative and realpath can fail in unusual
         * environments. Fall back to the current executable path.
         */
        if (getcwd(root, root_size) == NULL)
            return -1;

        return 0;
    }

    /*
     * executable:
     *
     * /.../chrono-os/init/chrono-init
     *
     * Remove:
     *
     * /init/chrono-init
     */
    char *init_dir = strrchr(executable, '/');

    if (!init_dir)
        return -1;

    *init_dir = '\0';

    char *root_dir = strrchr(executable, '/');

    if (!root_dir)
        return -1;

    *root_dir = '\0';

    if (strlen(executable) + 1 > root_size)
        return -1;

    strcpy(root, executable);

    return 0;
}

static int start_chronod(const char *service_path)
{
    pid_t pid = fork();

    if (pid < 0) {
        log_msg("ERROR", "fork() failed");
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        execl(service_path,
              service_path,
              (char *)NULL);

        perror("[INIT] exec");

        _exit(127);
    }

    chronod_pid = pid;

    printf("[INIT] Service started: %s (PID %d)\n",
           service_path,
           pid);

    fflush(stdout);

    log_msg("OK", "Service state: RUNNING");

    return 0;
}

static int supervise_chronod(const char *service_path)
{
    int restart_count = 0;

    while (!shutdown_requested) {

        log_msg("INIT", "Starting ChronoOS service supervisor");

        if (start_chronod(service_path) < 0)
            return -1;

        int status = 0;

        pid_t result;

        do {
            result = waitpid(chronod_pid,
                             &status,
                             0);
        } while (result < 0 && errno == EINTR);

        if (shutdown_requested) {
            break;
        }

        if (result < 0) {
            log_msg("ERROR", "waitpid() failed");
            perror("waitpid");
            return -1;
        }

        if (WIFEXITED(status)) {

            int exit_code = WEXITSTATUS(status);

            printf("[WARN] ChronoD exited with code %d\n",
                   exit_code);

        } else if (WIFSIGNALED(status)) {

            int signal_number = WTERMSIG(status);

            printf("[WARN] ChronoD terminated by signal %d\n",
                   signal_number);

        }

        chronod_pid = -1;

        if (restart_count >= MAX_RESTARTS) {
            log_msg("ERROR",
                    "ChronoD restart limit exceeded");

            return -1;
        }

        restart_count++;

        printf("[WARN] Restarting ChronoD (%d/%d)\n",
               restart_count,
               MAX_RESTARTS);

        sleep(RESTART_DELAY);
    }

    return 0;
}

static void shutdown_chronod(void)
{
    if (chronod_pid <= 0)
        return;

    log_msg("INIT", "Stopping ChronoD...");

    kill(chronod_pid, SIGTERM);

    int status;

    while (waitpid(chronod_pid, &status, 0) < 0) {
        if (errno == EINTR)
            continue;

        break;
    }

    chronod_pid = -1;

    log_msg("OK", "ChronoD stopped");
}

int main(int argc, char **argv)
{
    ChronoServiceRegistry registry;

    int service_count = service_registry_load(
        "../etc/chrono/services.conf",
        &registry
    );

    if (service_count < 0) {
        chrono_log("ERROR", "INIT", "Unable to load service registry");
        return 1;
    }

    chrono_log("OK", "INIT", "Service registry loaded");

    for (int i = 0; i < registry.count; i++) {
        printf("[SERVICE] %s | %s | %s | max=%d\n",
               registry.services[i].name,
               registry.services[i].executable,
               registry.services[i].restart_policy,
               registry.services[i].max_restarts);
    }


    (void)argc;

    printf("\n");
    printf("==============================================\n");
    printf("        ChronoOS Init %s\n", CHRONO_VERSION);
    printf("==============================================\n");
    printf("\n");

    printf("[INIT] Development mode: PID %d\n",
           getpid());

    if (install_signal_handlers() < 0) {
        perror("[INIT] signal setup");
        return EXIT_FAILURE;
    }

    log_msg("INIT", "Initializing runtime");

    char project_root[PATH_MAX];

    if (get_project_root(argv[0],
                         project_root,
                         sizeof(project_root)) < 0) {

        log_msg("ERROR",
                "Unable to determine ChronoOS project root");

        return EXIT_FAILURE;
    }

    log_msg("OK", "Runtime initialized");

    char service_path[PATH_MAX];

    int written = snprintf(
        service_path,
        sizeof(service_path),
        "%s/services/chronod/chronod",
        project_root
    );

    if (written < 0 ||
        (size_t)written >= sizeof(service_path)) {

        log_msg("ERROR",
                "ChronoD service path is too long");

        return EXIT_FAILURE;
    }

    if (access(service_path, X_OK) != 0) {

        fprintf(stderr,
                "[ERROR] ChronoD executable unavailable: %s\n",
                service_path);

        perror("[INIT] access");

        return EXIT_FAILURE;
    }

    log_msg("INIT",
            "Starting ChronoOS service supervisor");

    int result = supervise_chronod(service_path);

    shutdown_chronod();

    if (result == 0) {

        log_msg("OK",
                "ChronoOS shutdown complete");

        return EXIT_SUCCESS;
    }

    log_msg("ERROR",
            "ChronoOS stopped because of service failure");

    return EXIT_FAILURE;
}











