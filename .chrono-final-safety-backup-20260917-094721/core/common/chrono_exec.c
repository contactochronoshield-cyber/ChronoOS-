#define _POSIX_C_SOURCE 200809L

#include "chrono_exec.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int chrono_exec_direct(const char *program, char *const argv[])
{
    if (program == NULL || argv == NULL || argv[0] == NULL) {
        errno = EINVAL;
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        execvp(program, argv);

        fprintf(
            stderr,
            "[CHRONO] execvp(%s): %s\n",
            program,
            strerror(errno));

        _exit(127);
    }

    int status = 0;

    for (;;) {
        if (waitpid(pid, &status, 0) >= 0) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return -1;
}

int chrono_ledger_append(
    const char *event_type,
    const char *details)
{
    if (event_type == NULL || details == NULL ||
        *event_type == '\0' || *details == '\0') {
        errno = EINVAL;
        return -1;
    }

    char *const argv[] = {
        "./bin/chrono-ledger",
        "append",
        (char *)event_type,
        (char *)details,
        NULL
    };

    return chrono_exec_direct(
        "./bin/chrono-ledger",
        argv);
}
