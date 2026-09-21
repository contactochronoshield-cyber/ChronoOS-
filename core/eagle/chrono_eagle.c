#include "chrono_eagle.h"

#include <stdio.h>
#include <string.h>

static const char *generation_name(eagle_generation_t generation)
{
    switch (generation) {
        case EAGLE_GENERATION_LEGACY:
            return "LEGACY";
        case EAGLE_GENERATION_MODERN:
            return "MODERN";
        default:
            return "UNKNOWN";
    }
}

static const char *state_name(eagle_state_t state)
{
    switch (state) {
        case EAGLE_STATE_VERIFIED:
            return "VERIFIED";
        case EAGLE_STATE_WARNING:
            return "WARNING";
        case EAGLE_STATE_INCIDENT:
            return "INCIDENT";
        default:
            return "UNKNOWN";
    }
}

int eagle_init(void)
{
    printf("CHRONO EAGLE\n");
    printf("VERSION=%s\n", CHRONO_EAGLE_VERSION);
    printf("STATUS=INITIALIZED\n");

    return 0;
}

int eagle_identify(eagle_camera_t *camera)
{
    if (camera == NULL)
        return -1;

    if (camera->id[0] == '\0')
        return -1;

    /*
     * Generation classification is intentionally conservative.
     * Unknown firmware/vendor information must not be treated as
     * modern or secure by default.
     */
    if (camera->firmware[0] == '\0' ||
        camera->onvif_detected == 0) {
        camera->generation = EAGLE_GENERATION_UNKNOWN;
    } else if (camera->tls_verified &&
               camera->authentication_verified) {
        camera->generation = EAGLE_GENERATION_MODERN;
    } else {
        camera->generation = EAGLE_GENERATION_LEGACY;
    }

    return 0;
}

int eagle_assess_security(eagle_camera_t *camera)
{
    if (camera == NULL)
        return -1;

    if (!camera->onvif_detected &&
        !camera->rtsp_detected) {
        camera->state = EAGLE_STATE_UNKNOWN;
        return 0;
    }

    /*
     * Security posture is deliberately conservative:
     * absence of verified TLS/authentication cannot be interpreted
     * as a secure state.
     */
    if (camera->tls_verified &&
        camera->authentication_verified) {
        camera->state = EAGLE_STATE_VERIFIED;
    } else {
        camera->state = EAGLE_STATE_WARNING;
    }

    return 0;
}

int eagle_assess_physical(eagle_camera_t *camera)
{
    if (camera == NULL)
        return -1;

    /*
     * Values:
     *  0 = not assessed
     *  1 = normal
     *  2 = degraded
     *  3 = critical
     *
     * Physical assessment is observational only.
     */
    if (camera->optical_health >= 3 ||
        camera->alignment_health >= 3 ||
        camera->vibration_health >= 3) {
        camera->state = EAGLE_STATE_INCIDENT;
    } else if (camera->optical_health == 2 ||
               camera->alignment_health == 2 ||
               camera->vibration_health == 2) {
        if (camera->state != EAGLE_STATE_INCIDENT)
            camera->state = EAGLE_STATE_WARNING;
    }

    return 0;
}

int eagle_verify(eagle_camera_t *camera)
{
    if (camera == NULL)
        return -1;

    if (eagle_identify(camera) != 0)
        return -1;

    if (eagle_assess_security(camera) != 0)
        return -1;

    if (eagle_assess_physical(camera) != 0)
        return -1;

    return 0;
}

void eagle_print_report(const eagle_camera_t *camera)
{
    if (camera == NULL)
        return;

    printf("=== CHRONO EAGLE CAMERA REPORT ===\n");
    printf("ID=%s\n", camera->id);
    printf("VENDOR=%s\n", camera->vendor);
    printf("MODEL=%s\n", camera->model);
    printf("FIRMWARE=%s\n", camera->firmware);
    printf("GENERATION=%s\n", generation_name(camera->generation));

    printf("ONVIF=%s\n",
           camera->onvif_detected ? "DETECTED" : "NOT_DETECTED");

    printf("TLS=%s\n",
           camera->tls_verified ? "VERIFIED" : "NOT_VERIFIED");

    printf("AUTHENTICATION=%s\n",
           camera->authentication_verified ? "VERIFIED" : "NOT_VERIFIED");

    printf("RTSP=%s\n",
           camera->rtsp_detected ? "DETECTED" : "NOT_DETECTED");

    printf("OPTICAL_HEALTH=%d\n", camera->optical_health);
    printf("ALIGNMENT_HEALTH=%d\n", camera->alignment_health);
    printf("VIBRATION_HEALTH=%d\n", camera->vibration_health);

    printf("STATE=%s\n", state_name(camera->state));
}
