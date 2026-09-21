#include "chrono_vertical_device.h"

#include <stdio.h>
#include <string.h>

static const char *device_type_name(
    chrono_vertical_device_type_t type
)
{
    switch (type) {
        case CHRONO_VERTICAL_DEVICE_CAMERA:
            return "CAMERA";

        case CHRONO_VERTICAL_DEVICE_ROUTER:
            return "ROUTER";

        case CHRONO_VERTICAL_DEVICE_SENSOR:
            return "SENSOR";

        default:
            return "UNKNOWN";
    }
}

static const char *device_status_name(
    chrono_vertical_device_status_t status
)
{
    switch (status) {
        case CHRONO_VERTICAL_DEVICE_ONLINE:
            return "ONLINE";

        default:
            return "OFFLINE";
    }
}

void chrono_vertical_device_init(
    chrono_vertical_device_t *device
)
{
    if (device == NULL)
        return;

    memset(device, 0, sizeof(*device));

    device->type =
        CHRONO_VERTICAL_DEVICE_UNKNOWN;

    device->status =
        CHRONO_VERTICAL_DEVICE_OFFLINE;
}

int chrono_vertical_device_register(
    chrono_vertical_device_t *device,
    const char *id,
    chrono_vertical_device_type_t type,
    int floor,
    const char *zone,
    const char *vendor,
    const char *model,
    const char *firmware
)
{
    if (device == NULL ||
        id == NULL ||
        zone == NULL ||
        vendor == NULL ||
        model == NULL ||
        firmware == NULL) {
        return -1;
    }

    if (id[0] == '\0' ||
        zone[0] == '\0' ||
        floor < 1) {
        return -1;
    }

    if (type == CHRONO_VERTICAL_DEVICE_UNKNOWN)
        return -1;

    chrono_vertical_device_init(device);

    snprintf(
        device->id,
        sizeof(device->id),
        "%s",
        id
    );

    device->type = type;
    device->floor = floor;

    snprintf(
        device->zone,
        sizeof(device->zone),
        "%s",
        zone
    );

    snprintf(
        device->vendor,
        sizeof(device->vendor),
        "%s",
        vendor
    );

    snprintf(
        device->model,
        sizeof(device->model),
        "%s",
        model
    );

    snprintf(
        device->firmware,
        sizeof(device->firmware),
        "%s",
        firmware
    );

    device->status =
        CHRONO_VERTICAL_DEVICE_ONLINE;

    return 0;
}

void chrono_vertical_device_print(
    const chrono_vertical_device_t *device
)
{
    if (device == NULL)
        return;

    printf("DEVICE_ID=%s\n", device->id);
    printf(
        "DEVICE_TYPE=%s\n",
        device_type_name(device->type)
    );
    printf("FLOOR=%d\n", device->floor);
    printf("ZONE=%s\n", device->zone);
    printf("VENDOR=%s\n", device->vendor);
    printf("MODEL=%s\n", device->model);
    printf("FIRMWARE=%s\n", device->firmware);
    printf(
        "STATUS=%s\n",
        device_status_name(device->status)
    );
}
