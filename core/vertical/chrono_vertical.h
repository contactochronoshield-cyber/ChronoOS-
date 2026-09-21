#ifndef CHRONO_VERTICAL_H
#define CHRONO_VERTICAL_H

#define CHRONO_VERTICAL_VERSION "1.0"

#define CHRONO_VERTICAL_BUILDING_NAME_MAX 128
#define CHRONO_VERTICAL_TOWER_NAME_MAX 64
#define CHRONO_VERTICAL_ZONE_NAME_MAX 64

typedef struct {
    int floor_number;
    char zone[CHRONO_VERTICAL_ZONE_NAME_MAX];
    int cameras;
    int network_nodes;
    int sensors;
} chrono_vertical_floor_t;

typedef struct {
    char name[CHRONO_VERTICAL_BUILDING_NAME_MAX];
    char tower[CHRONO_VERTICAL_TOWER_NAME_MAX];

    int total_floors;
    int total_cameras;
    int total_network_nodes;
    int total_sensors;
} chrono_vertical_building_t;

int chrono_vertical_init(void);

int chrono_vertical_building_init(
    chrono_vertical_building_t *building,
    const char *name,
    const char *tower,
    int total_floors
);

int chrono_vertical_floor_init(
    chrono_vertical_floor_t *floor,
    int floor_number,
    const char *zone
);

int chrono_vertical_register_floor(
    chrono_vertical_building_t *building,
    const chrono_vertical_floor_t *floor
);

void chrono_vertical_print_report(
    const chrono_vertical_building_t *building
);

#endif
