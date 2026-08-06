#include <stdio.h>
#include <string.h>
#include "chronoctl.h"

int main(int argc, char *argv[])
{
    printf("ChronoCTL %s\n", CHRONOCTL_VERSION);

    if (argc < 2) {
        printf("\nUso:\n");
        printf("  chronoctl version\n");
        printf("  chronoctl status\n");
        printf("  chronoctl services\n");
        return 0;
    }

    if (strcmp(argv[1], "version") == 0) {
        printf("ChronoOS 1.0.0-alpha\n");
    } else if (strcmp(argv[1], "status") == 0) {
        printf("Sistema: Operativo\n");
    } else if (strcmp(argv[1], "services") == 0) {
        printf("chronod: activo\n");
    } else {
        printf("Comando desconocido: %s\n", argv[1]);
    }

    return 0;
}
