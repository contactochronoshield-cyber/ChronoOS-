#include "chrono_shell_guard.h"

#include <errno.h>

int chrono_system_disabled(const char *command)
{
    (void)command;
    errno = EPERM;
    return -1;
}

FILE *chrono_popen_disabled(const char *command, const char *mode)
{
    (void)command;
    (void)mode;
    errno = EPERM;
    return NULL;
}
