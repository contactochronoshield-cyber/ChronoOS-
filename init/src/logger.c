#define _POSIX_C_SOURCE 200809L

#include "logger.h"

#include <stdio.h>
#include <time.h>

#define LOG_FILE "var/logs/chrono.log"

void chrono_log(
    const char *level,
    const char *component,
    const char *message
)
{
    FILE *log = fopen(LOG_FILE, "a");

    if (!log)
        return;

    time_t now = time(NULL);

    struct tm tm_now;

    localtime_r(
        &now,
        &tm_now
    );

    char timestamp[32];

    strftime(
        timestamp,
        sizeof(timestamp),
        "%Y-%m-%d %H:%M:%S",
        &tm_now
    );

    fprintf(
        log,
        "%s [%s] [%s] %s\n",
        timestamp,
        level,
        component,
        message
    );

    fclose(log);
}
