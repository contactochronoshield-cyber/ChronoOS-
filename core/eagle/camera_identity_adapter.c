#include "camera_identity.h"
#include "chrono_eagle.h"

#include <stdio.h>
#include <string.h>

int chrono_camera_identity_from_discovery(
    chrono_camera_identity_t *identity,
    const char *types,
    const char *xaddr
)
{
    if (identity == NULL || types == NULL || xaddr == NULL)
        return -1;

    return chrono_camera_identity_from_onvif(
        identity,
        types,
        xaddr
    );
}

int chrono_camera_identity_to_eagle(
    const chrono_camera_identity_t *identity,
    eagle_camera_t *camera,
    const char *camera_id
)
{
    if (identity == NULL || camera == NULL || camera_id == NULL)
        return -1;

    if (!identity->onvif)
        return -1;

    memset(camera, 0, sizeof(*camera));

    snprintf(
        camera->id,
        sizeof(camera->id),
        "%s",
        camera_id
    );

    snprintf(
        camera->vendor,
        sizeof(camera->vendor),
        "%s",
        identity->vendor
    );

    snprintf(
        camera->model,
        sizeof(camera->model),
        "%s",
        identity->model
    );

    snprintf(
        camera->firmware,
        sizeof(camera->firmware),
        "%s",
        identity->firmware
    );

    camera->onvif_detected =
        identity->onvif;

    camera->tls_verified =
        identity->https;

    camera->authentication_verified =
        identity->authenticated;

    camera->rtsp_detected =
        identity->rtsp;

    return 0;
}
