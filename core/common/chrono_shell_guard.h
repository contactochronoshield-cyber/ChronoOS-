#ifndef CHRONO_SHELL_GUARD_H
#define CHRONO_SHELL_GUARD_H

#include <stdio.h>

int chrono_system_disabled(const char *command);
FILE *chrono_popen_disabled(const char *command, const char *mode);

#endif
