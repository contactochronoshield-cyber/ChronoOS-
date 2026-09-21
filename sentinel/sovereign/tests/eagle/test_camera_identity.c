#include <stdio.h>

#include "camera_identity.h"

int main(void)
{
    chrono_camera_identity_t identity;

    if (chrono_camera_identity_from_onvif(
            &identity,
            "dn:NetworkVideoTransmitter",
            "http://127.0.0.1/onvif/device_service"
        ) != 0) {
        printf("CAMERA_IDENTITY=FAIL\n");
        return 1;
    }

    if (!identity.onvif) {
        printf("CAMERA_IDENTITY_ONVIF=FAIL\n");
        return 1;
    }

    if (!identity.http) {
        printf("CAMERA_IDENTITY_HTTP=FAIL\n");
        return 1;
    }

    if (identity.https) {
        printf("CAMERA_IDENTITY_HTTPS=FAIL\n");
        return 1;
    }

    printf("CHRONO CAMERA IDENTITY\n");
    printf("VENDOR=%s\n", identity.vendor);
    printf("MODEL=%s\n", identity.model);
    printf("FIRMWARE=%s\n", identity.firmware);
    printf("PROTOCOL=%s\n", identity.protocol);
    printf("ONVIF=%d\n", identity.onvif);
    printf("HTTP=%d\n", identity.http);
    printf("HTTPS=%d\n", identity.https);
    printf("SERVICE_URL=%s\n", identity.service_url);
    printf("CAMERA_IDENTITY=PASS\n");

    return 0;
}
