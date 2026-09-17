#ifndef CHRONO_EXEC_H
#define CHRONO_EXEC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Executes an executable directly with argv[].
 *
 * No shell is invoked.
 * No system().
 * No popen().
 *
 * Returns the child exit status or -1 on execution/wait failure.
 */
int chrono_exec_direct(const char *program, char *const argv[]);

/*
 * Appends an event to the Chrono Ledger without shell interpretation.
 */
int chrono_ledger_append(const char *event_type, const char *details);

#ifdef __cplusplus
}
#endif

#endif
