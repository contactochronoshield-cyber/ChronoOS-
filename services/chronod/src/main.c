#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

static volatile sig_atomic_t running = 1;

static void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

static void timestamp(void)
{
    time_t now = time(NULL);
    struct tm tm_now;

    localtime_r(&now, &tm_now);

    printf("[%02d:%02d:%02d] ",
           tm_now.tm_hour,
           tm_now.tm_min,
           tm_now.tm_sec);
}

int main(void)
{
    struct sigaction sa;

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);

    timestamp();
    printf("[ChronoD] Initializing...\n");

    timestamp();
    printf("[ChronoD] Service manager started.\n");

    fflush(stdout);

    while (running) {
        sleep(1);
    }

    timestamp();
    printf("[ChronoD] Shutdown requested.\n");

    timestamp();
    printf("[ChronoD] Service manager stopped.\n");

    return 0;
}
