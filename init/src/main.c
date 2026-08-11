#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define CHRONO_VERSION "1.2.0-alpha"

#define MAX_RESTARTS 3
#define RESTART_DELAY 1

typedef enum {
    SERVICE_STOPPED = 0,
    SERVICE_STARTING,
    SERVICE_RUNNING,
    SERVICE_STOPPING,
    SERVICE_FAILED
} service_state_t;

static volatile sig_atomic_t shutdown_requested = 0;

static pid_t chronod_pid = -1;
static service_state_t chronod_state = SERVICE_STOPPED;

static char chronod_path[PATH_MAX];

static void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM) {
        shutdown_requested = 1;
    }
}

static const char *state_name(service_state_t state)
{
    switch (state) {
        case SERVICE_STOPPED:
            return "STOPPED";

        case SERVICE_STARTING:
            return "STARTING";

        case SERVICE_RUNNING:
            return "RUNNING";

        case SERVICE_STOPPING:
            return "STOPPING";

        case SERVICE_FAILED:
            return "FAILED";

        default:
            return "UNKNOWN";
    }
}

static void timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm tm_now;

    if (localtime_r(&now, &tm_now) == NULL) {
        snprintf(buffer, size, "00:00:00");
        return;
    }

    strftime(buffer, size, "%H:%M:%S", &tm_now);
}

static void log_line(const char *level, const char *message)
{
    char timebuf[32];

    timestamp(timebuf, sizeof(timebuf));

    printf("[%s] [%s] %s\n", timebuf, level, message);
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

    /*
     * No queremos que ChronoOS muera si un servicio
     * cierra accidentalmente un pipe/socket.
     */
    signal(SIGPIPE, SIG_IGN);

    return 0;
}

/*
 * Obtiene la ruta real del ejecutable chrono-init.
 *
 * Ejemplo:
 *
 * /home/chrono-os/init/chrono-init
 *
 * A partir de ahí obtiene:
 *
 * /home/chrono-os
 */
static int find_chrono_root(char *root, size_t size)
{
    char executable[PATH_MAX];
    char *init_dir;
    char *root_dir;

    ssize_t len = readlink(
        "/proc/self/exe",
        executable,
        sizeof(executable) - 1
    );

    if (len < 0) {
        perror("readlink /proc/self/exe");
        return -1;
    }

    executable[len] = '\0';

    /*
     * executable:
     * /chrono-os/init/chrono-init
     *
     * dirname -> /chrono-os/init
     * dirname -> /chrono-os
     */

    init_dir = strrchr(executable, '/');

    if (init_dir == NULL)
        return -1;

    *init_dir = '\0';

    root_dir = strrchr(executable, '/');

    if (root_dir == NULL)
        return -1;

    *root_dir = '\0';

    if (strlen(executable) + 1 > size)
        return -1;

    snprintf(root, size, "%s", executable);

    return 0;
}

static int build_service_path(void)
{
    char root[PATH_MAX];

    if (find_chrono_root(root, sizeof(root)) < 0) {
        log_line("ERROR", "Unable to determine ChronoOS root");
        return -1;
    }

    int written = snprintf(
        chronod_path,
        sizeof(chronod_path),
        "%s/services/chronod/chronod",
        root
    );

    if (written < 0 || (size_t)written >= sizeof(chronod_path)) {
        log_line("ERROR", "ChronoD path is too long");
        return -1;
    }

    if (access(chronod_path, X_OK) != 0) {
        fprintf(
            stderr,
            "[ERROR] ChronoD executable unavailable: %s\n",
            chronod_path
        );

        return -1;
    }

    return 0;
}

static void print_service_status(void)
{
    printf(
        "[INIT] ChronoD state: %s",
        state_name(chronod_state)
    );

    if (chronod_pid > 0)
        printf(" (PID %ld)", (long)chronod_pid);

    printf("\n");

    fflush(stdout);
}

static int start_chronod(void)
{
    pid_t pid;

    chronod_state = SERVICE_STARTING;

    log_line("INIT", "Starting ChronoD");

    pid = fork();

    if (pid < 0) {
        perror("[INIT] fork");
        chronod_state = SERVICE_FAILED;
        return -1;
    }

    if (pid == 0) {
        /*
         * El hijo se convierte en ChronoD.
         */
        execl(
            chronod_path,
            chronod_path,
            (char *)NULL
        );

        /*
         * Si llegamos aquí exec falló.
         */
        fprintf(
            stderr,
            "[INIT] exec failed for ChronoD: %s\n",
            strerror(errno)
        );

        _exit(127);
    }

    chronod_pid = pid;
    chronod_state = SERVICE_RUNNING;

    printf(
        "[INIT] ChronoD started: %s (PID %ld)\n",
        chronod_path,
        (long)chronod_pid
    );

    print_service_status();

    return 0;
}

static void stop_chronod(void)
{
    if (chronod_pid <= 0)
        return;

    chronod_state = SERVICE_STOPPING;

    log_line("INIT", "Stopping ChronoD");

    if (kill(chronod_pid, SIGTERM) < 0) {

        if (errno != ESRCH) {
            perror("[INIT] kill");
        }
    }

    /*
     * Esperamos que ChronoD termine.
     */
    for (int i = 0; i < 50; i++) {

        pid_t result = waitpid(
            chronod_pid,
            NULL,
            WNOHANG
        );

        if (result == chronod_pid) {
            chronod_pid = -1;
            chronod_state = SERVICE_STOPPED;

            log_line("OK", "ChronoD stopped");

            return;
        }

        if (result < 0) {

            if (errno == ECHILD) {
                chronod_pid = -1;
                chronod_state = SERVICE_STOPPED;

                log_line("OK", "ChronoD stopped");

                return;
            }

            break;
        }

        sleep(1);
    }

    /*
     * Si no respondió a SIGTERM, terminamos el proceso.
     */
    if (chronod_pid > 0) {

        log_line(
            "WARN",
            "ChronoD did not stop gracefully; sending SIGKILL"
        );

        if (kill(chronod_pid, SIGKILL) < 0) {
            if (errno != ESRCH)
                perror("[INIT] SIGKILL");
        }

        waitpid(chronod_pid, NULL, 0);

        chronod_pid = -1;
        chronod_state = SERVICE_STOPPED;

        log_line("OK", "ChronoD forcibly stopped");
    }
}

static int supervise(void)
{
    int restart_count = 0;

    while (!shutdown_requested) {

        if (chronod_pid <= 0) {

            if (start_chronod() < 0) {

                chronod_state = SERVICE_FAILED;

                log_line(
                    "ERROR",
                    "Unable to start ChronoD"
                );

                return 1;
            }
        }

        /*
         * Esperamos mientras ChronoD siga ejecutándose.
         */
        while (!shutdown_requested) {

            int status = 0;

            pid_t result = waitpid(
                chronod_pid,
                &status,
                WNOHANG
            );

            if (result == 0) {
                sleep(1);
                continue;
            }

            if (result < 0) {

                if (errno == EINTR)
                    continue;

                perror("[INIT] waitpid");

                chronod_state = SERVICE_FAILED;
                return 1;
            }

            /*
             * ChronoD terminó.
             */
            chronod_pid = -1;

            if (WIFEXITED(status)) {

                int code = WEXITSTATUS(status);

                printf(
                    "[INIT] ChronoD exited with code %d\n",
                    code
                );

            } else if (WIFSIGNALED(status)) {

                printf(
                    "[INIT] ChronoD terminated by signal %d\n",
                    WTERMSIG(status)
                );
            }

            chronod_state = SERVICE_STOPPED;

            break;
        }

        if (shutdown_requested)
            break;

        restart_count++;

        if (restart_count > MAX_RESTARTS) {

            log_line(
                "ERROR",
                "ChronoD restart limit exceeded"
            );

            chronod_state = SERVICE_FAILED;

            return 1;
        }

        printf(
            "[WARN] Restarting ChronoD (%d/%d)\n",
            restart_count,
            MAX_RESTARTS
        );

        sleep(RESTART_DELAY);
    }

    return 0;
}

static void print_banner(void)
{
    printf("\n");
    printf("==============================================\n");
    printf("          ChronoOS Init %s\n", CHRONO_VERSION);
    printf("==============================================\n");
    printf("\n");

    fflush(stdout);
}

int main(void)
{
    int result;

    print_banner();

    printf(
        "[INIT] Development mode: PID %ld\n",
        (long)getpid()
    );

    if (install_signal_handlers() < 0) {

        perror("[ERROR] signal setup");
        return EXIT_FAILURE;
    }

    log_line(
        "INIT",
        "Initializing ChronoOS runtime"
    );

    /*
     * Determinamos la ubicación real de ChronoOS.
     */
    if (build_service_path() < 0) {

        log_line(
            "ERROR",
            "ChronoOS runtime initialization failed"
        );

        return EXIT_FAILURE;
    }

    log_line(
        "OK",
        "Runtime initialized"
    );

    printf(
        "[INIT] Service path: %s\n",
        chronod_path
    );

    log_line(
        "INIT",
        "Starting ChronoOS service supervisor"
    );

    result = supervise();

    /*
     * Shutdown ordenado.
     */
    if (chronod_pid > 0)
        stop_chronod();

    if (result == 0) {

        log_line(
            "OK",
            "ChronoOS shutdown complete"
        );

        return EXIT_SUCCESS;
    }

    log_line(
        "ERROR",
        "ChronoOS stopped because of service failure"
    );

    return EXIT_FAILURE;
}
