#ifndef CHRONO_EXEC_H
#define CHRONO_EXEC_H

#ifdef __cplusplus
extern "C" {
#endif

int chrono_exec_direct(const char *program, char *const argv[]);
int chrono_ledger_append(const char *event_type, const char *details);

#ifdef __cplusplus
}
#endif

#endif
