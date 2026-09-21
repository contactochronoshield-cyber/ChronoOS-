#include <stdio.h>
#include <string.h>

#include "camera_identity.h"
#include "chrono_eagle.h"

int main(void)
{
    chrono_camera_identity_t identity;
    eagle_camera_t camera;

    if (chrono_camera_identity_from_discovery(
            &identity,
            "dn:NetworkVideoTransmitter",
            "http://127.0.0.1/onvif/device_service"
        ) != 0) {
        printf("DISCOVERY=FAIL\n");
        return 1;
    }

    if (chrono_camera_identity_to_eagle(
            &identity,
            &camera,
            "EAGLE-SIM-001"
        ) != 0) {
        printf("IDENTITY_TO_EAGLE=FAIL\n");
        return 1;
    }

    if (eagle_verify(&camera) != 0) {
        printf("EAGLE_VERIFY=FAIL\n");
        return 1;
    }

    printf("=== DISCOVERY → IDENTITY → EAGLE ===\n");
    printf("DISCOVERY=PASS\n");
    printf("IDENTITY=PASS\n");
    printf("EAGLE_VERIFY=PASS\n");

    eagle_print_report(&camera);

    if (camera.state != EAGLE_STATE_WARNING) {
        printf("EXPECTED_STATE=WARNING\n");
        printf("INTEGRATION_RESULT=FAIL\n");
        return 1;
    }

    printf("EXPECTED_STATE=WARNING\n");
    printf("INTEGRATION_RESULT=PASS\n");

    return 0;
}
