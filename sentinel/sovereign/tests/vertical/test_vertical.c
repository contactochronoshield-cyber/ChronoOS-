#include <stdio.h>

#include "chrono_vertical.h"

int main(void)
{
    chrono_vertical_building_t building;
    chrono_vertical_floor_t floor;

    if (chrono_vertical_init() != 0)
        return 1;

    if (chrono_vertical_building_init(
            &building,
            "SUPERTALL-SIMULATOR",
            "TOWER-A",
            163
        ) != 0) {
        printf("BUILDING_INIT=FAIL\n");
        return 1;
    }

    if (chrono_vertical_floor_init(
            &floor,
            37,
            "EAST"
        ) != 0) {
        printf("FLOOR_INIT=FAIL\n");
        return 1;
    }

    floor.cameras = 12;
    floor.network_nodes = 3;
    floor.sensors = 25;

    if (chrono_vertical_register_floor(&building, &floor) != 0) {
        printf("FLOOR_REGISTER=FAIL\n");
        return 1;
    }

    if (building.total_cameras != 12 ||
        building.total_network_nodes != 3 ||
        building.total_sensors != 25) {
        printf("COUNTERS=FAIL\n");
        return 1;
    }

    printf("BUILDING_INIT=PASS\n");
    printf("FLOOR_INIT=PASS\n");
    printf("FLOOR_REGISTER=PASS\n");
    printf("COUNTERS=PASS\n");

    chrono_vertical_print_report(&building);

    printf("CHRONO_VERTICAL=PASS\n");

    return 0;
}
