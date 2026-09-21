#ifndef CHRONO_EAGLE_H
#define CHRONO_EAGLE_H

#define CHRONO_EAGLE_VERSION "1.0"

typedef enum {
    EAGLE_GENERATION_UNKNOWN = 0,
    EAGLE_GENERATION_LEGACY,
    EAGLE_GENERATION_MODERN
} eagle_generation_t;

typedef enum {
    EAGLE_STATE_UNKNOWN = 0,
    EAGLE_STATE_VERIFIED,
    EAGLE_STATE_WARNING,
    EAGLE_STATE_INCIDENT
} eagle_state_t;

typedef struct {
    char id[64];
    char vendor[128];
    char model[128];
    char firmware[128];

    eagle_generation_t generation;

    int onvif_detected;
    int tls_verified;
    int authentication_verified;
    int rtsp_detected;

    int optical_health;
    int alignment_health;
    int vibration_health;

    eagle_state_t state;
} eagle_camera_t;

int eagle_init(void);

int eagle_identify(
    eagle_camera_t *camera
);

int eagle_assess_security(
    eagle_camera_t *camera
);

int eagle_assess_physical(
    eagle_camera_t *camera
);

int eagle_verify(
    eagle_camera_t *camera
);

void eagle_print_report(
    const eagle_camera_t *camera
);

#endif
