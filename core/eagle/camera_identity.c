#include "camera_identity.h"

#include <stdio.h>
#include <string.h>

void chrono_camera_identity_init(chrono_camera_identity_t *identity)
{
    if (identity == NULL)
        return;

    memset(identity, 0, sizeof(*identity));

    snprintf(
        identity->vendor,
        sizeof(identity->vendor),
        "Unknown"
    );

    snprintf(
        identity->model,
        sizeof(identity->model),
        "Unknown"
    );

    snprintf(
        identity->firmware,
        sizeof(identity->firmware),
        "Unknown"
    );

    snprintf(
        identity->protocol,
        sizeof(identity->protocol),
        "ONVIF"
    );
}

int chrono_camera_identity_from_onvif(
    chrono_camera_identity_t *identity,
    const char *types,
    const char *xaddr
)
{
    if (identity == NULL || types == NULL || xaddr == NULL)
        return -1;

    chrono_camera_identity_init(identity);

    if (strstr(types, "NetworkVideoTransmitter") == NULL)
        return -1;

    identity->onvif = 1;

    if (strstr(xaddr, "https://") == xaddr)
        identity->https = 1;
    else if (strstr(xaddr, "http://") == xaddr)
        identity->http = 1;

    snprintf(
        identity->service_url,
        sizeof(identity->service_url),
        "%s",
        xaddr
    );

    return 0;
}
