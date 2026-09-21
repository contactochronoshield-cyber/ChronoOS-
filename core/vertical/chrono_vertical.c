#include "chrono_vertical.h"

#include <stdio.h>
#include <string.h>

int chrono_vertical_init(void)
{
    printf("CHRONO VERTICAL\n");
    printf("VERSION=%s\n", CHRONO_VERTICAL_VERSION);
    printf("STATUS=INITIALIZED\n");

    return 0;
}

int chrono_vertical_building_init(
    chrono_vertical_building_t *building,
    const char *name,
    const char *tower,
    int total_floors
)
{
    if (building == NULL ||
        name == NULL ||
        tower == NULL ||
        total_floors <= 0) {
        return -1;
    }

    memset(building, 0, sizeof(*building));

    snprintf(
        building->name,
        sizeof(building->name),
        "%s",
        name
    );

    snprintf(
        building->tower,
        sizeof(building->tower),
        "%s",
        tower
    );

    building->total_floors = total_floors;

    return 0;
}

int chrono_vertical_floor_init(
    chrono_vertical_floor_t *floor,
    int floor_number,
    const char *zone
)
{
    if (floor == NULL ||
        zone == NULL ||
        floor_number <= 0) {
        return -1;
    }

    memset(floor, 0, sizeof(*floor));

    floor->floor_number = floor_number;

    snprintf(
        floor->zone,
        sizeof(floor->zone),
        "%s",
        zone
    );

    return 0;
}

int chrono_vertical_register_floor(
    chrono_vertical_building_t *building,
    const chrono_vertical_floor_t *floor
)
{
    if (building == NULL || floor == NULL)
        return -1;

    if (floor->floor_number <= 0 ||
        floor->floor_number > building->total_floors) {
        return -1;
    }

    if (floor->cameras < 0 ||
        floor->network_nodes < 0 ||
        floor->sensors < 0) {
        return -1;
    }

    building->total_cameras += floor->cameras;
    building->total_network_nodes += floor->network_nodes;
    building->total_sensors += floor->sensors;

    return 0;
}

void chrono_vertical_print_report(
    const chrono_vertical_building_t *building
)
{
    if (building == NULL)
        return;

    printf("=== CHRONO VERTICAL BUILDING REPORT ===\n");
    printf("BUILDING=%s\n", building->name);
    printf("TOWER=%s\n", building->tower);
    printf("FLOORS=%d\n", building->total_floors);
    printf("CAMERAS=%d\n", building->total_cameras);
    printf("NETWORK_NODES=%d\n", building->total_network_nodes);
    printf("SENSORS=%d\n", building->total_sensors);
}
