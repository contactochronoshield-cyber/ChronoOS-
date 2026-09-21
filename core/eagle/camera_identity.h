#ifndef CHRONO_CAMERA_IDENTITY_H
#define CHRONO_CAMERA_IDENTITY_H

#include "chrono_eagle.h"

#define CHRONO_CAMERA_VENDOR_MAX 64
#define CHRONO_CAMERA_MODEL_MAX 64
#define CHRONO_CAMERA_FIRMWARE_MAX 64
#define CHRONO_CAMERA_PROTOCOL_MAX 32
#define CHRONO_CAMERA_XADDR_MAX 256

typedef struct {
    char vendor[CHRONO_CAMERA_VENDOR_MAX];
    char model[CHRONO_CAMERA_MODEL_MAX];
    char firmware[CHRONO_CAMERA_FIRMWARE_MAX];
    char protocol[CHRONO_CAMERA_PROTOCOL_MAX];
    char service_url[CHRONO_CAMERA_XADDR_MAX];

    int onvif;
    int rtsp;
    int http;
    int https;
    int authenticated;
} chrono_camera_identity_t;

void chrono_camera_identity_init(chrono_camera_identity_t *identity);

int chrono_camera_identity_from_onvif(
    chrono_camera_identity_t *identity,
    const char *types,
    const char *xaddr
);

int chrono_camera_identity_from_discovery(
    chrono_camera_identity_t *identity,
    const char *types,
    const char *xaddr
);

#endif

int chrono_camera_identity_to_eagle(
    const chrono_camera_identity_t *identity,
    eagle_camera_t *camera,
    const char *camera_id
);
