#ifndef CHRONO_VERTICAL_DEVICE_H
#define CHRONO_VERTICAL_DEVICE_H

#define CHRONO_VERTICAL_DEVICE_VERSION "1.0"

#define CHRONO_VERTICAL_DEVICE_ID_MAX 64
#define CHRONO_VERTICAL_DEVICE_VENDOR_MAX 64
#define CHRONO_VERTICAL_DEVICE_MODEL_MAX 64
#define CHRONO_VERTICAL_DEVICE_FIRMWARE_MAX 64
#define CHRONO_VERTICAL_DEVICE_ZONE_MAX 64

typedef enum {
    CHRONO_VERTICAL_DEVICE_UNKNOWN = 0,
    CHRONO_VERTICAL_DEVICE_CAMERA,
    CHRONO_VERTICAL_DEVICE_ROUTER,
    CHRONO_VERTICAL_DEVICE_SENSOR
} chrono_vertical_device_type_t;

typedef enum {
    CHRONO_VERTICAL_DEVICE_OFFLINE = 0,
    CHRONO_VERTICAL_DEVICE_ONLINE
} chrono_vertical_device_status_t;

typedef struct {
    char id[CHRONO_VERTICAL_DEVICE_ID_MAX];

    chrono_vertical_device_type_t type;

    int floor;
    char zone[CHRONO_VERTICAL_DEVICE_ZONE_MAX];

    char vendor[CHRONO_VERTICAL_DEVICE_VENDOR_MAX];
    char model[CHRONO_VERTICAL_DEVICE_MODEL_MAX];
    char firmware[CHRONO_VERTICAL_DEVICE_FIRMWARE_MAX];

    chrono_vertical_device_status_t status;
} chrono_vertical_device_t;

void chrono_vertical_device_init(
    chrono_vertical_device_t *device
);

int chrono_vertical_device_register(
    chrono_vertical_device_t *device,
    const char *id,
    chrono_vertical_device_type_t type,
    int floor,
    const char *zone,
    const char *vendor,
    const char *model,
    const char *firmware
);

void chrono_vertical_device_print(
    const chrono_vertical_device_t *device
);

#endif
