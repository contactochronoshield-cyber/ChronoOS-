#ifndef CHRONO_CONTINUUM_H
#define CHRONO_CONTINUUM_H

#define CHRONO_CONTINUUM_VERSION "1.0"

int continuum_init(const char *root);

int continuum_event(
    const char *root,
    const char *event_type,
    const char *actor,
    const char *details
);

int continuum_verify(const char *root);

int continuum_state(const char *root);

int continuum_passport(const char *root);

#endif
