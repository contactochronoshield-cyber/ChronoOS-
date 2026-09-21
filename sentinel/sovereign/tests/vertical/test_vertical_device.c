#include <stdio.h>
#include <string.h>

#include "chrono_vertical_device.h"

int main(void)
{
    chrono_vertical_device_t camera;
    chrono_vertical_device_t router;
    chrono_vertical_device_t sensor;

    if (chrono_vertical_device_register(
            &camera,
            "CAM-037-001",
            CHRONO_VERTICAL_DEVICE_CAMERA,
            37,
            "EAST",
            "Chrono-Simulator",
            "Eagle-Cam-01",
            "1.0"
        ) != 0) {
        printf("CAMERA_REGISTER=FAIL\n");
        return 1;
    }

    if (chrono_vertical_device_register(
            &router,
            "RTR-037-001",
            CHRONO_VERTICAL_DEVICE_ROUTER,
            37,
            "NETWORK",
            "Chrono-Simulator",
            "Vertical-Router-01",
            "1.0"
        ) != 0) {
        printf("ROUTER_REGISTER=FAIL\n");
        return 1;
    }

    if (chrono_vertical_device_register(
            &sensor,
            "SNS-037-001",
            CHRONO_VERTICAL_DEVICE_SENSOR,
            37,
            "MECHANICAL",
            "Chrono-Simulator",
            "Vertical-Sensor-01",
            "1.0"
        ) != 0) {
        printf("SENSOR_REGISTER=FAIL\n");
        return 1;
    }

    if (camera.type != CHRONO_VERTICAL_DEVICE_CAMERA ||
        router.type != CHRONO_VERTICAL_DEVICE_ROUTER ||
        sensor.type != CHRONO_VERTICAL_DEVICE_SENSOR) {
        printf("DEVICE_TYPES=FAIL\n");
        return 1;
    }

    if (camera.floor != 37 ||
        router.floor != 37 ||
        sensor.floor != 37) {
        printf("FLOOR_ASSIGNMENT=FAIL\n");
        return 1;
    }

    if (camera.status != CHRONO_VERTICAL_DEVICE_ONLINE ||
        router.status != CHRONO_VERTICAL_DEVICE_ONLINE ||
        sensor.status != CHRONO_VERTICAL_DEVICE_ONLINE) {
        printf("DEVICE_STATUS=FAIL\n");
        return 1;
    }

    printf("CAMERA_REGISTER=PASS\n");
    printf("ROUTER_REGISTER=PASS\n");
    printf("SENSOR_REGISTER=PASS\n");
    printf("DEVICE_TYPES=PASS\n");
    printf("FLOOR_ASSIGNMENT=PASS\n");
    printf("DEVICE_STATUS=PASS\n");

    printf("=== CHRONO VERTICAL DEVICE REGISTRY ===\n");

    chrono_vertical_device_print(&camera);
    chrono_vertical_device_print(&router);
    chrono_vertical_device_print(&sensor);

    printf("VERTICAL_DEVICE_REGISTRY=PASS\n");

    return 0;
}
