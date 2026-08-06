#include <stdio.h>
#include <stdlib.h>

#define CHRONO_INIT_VERSION "1.0.0-alpha"

int main(void)
{
    printf("\n");
    printf("=========================================\n");
    printf("          ChronoOS Boot Manager\n");
    printf("=========================================\n");
    printf("Version : %s\n", CHRONO_INIT_VERSION);
    printf("Estado  : Inicializando sistema...\n\n");

    printf("[1/6] Cargando configuracion...\n");
    printf("[2/6] Inicializando kernel...\n");
    printf("[3/6] Montando sistemas de archivos...\n");
    printf("[4/6] Inicializando seguridad...\n");
    printf("[5/6] Iniciando ChronoD...\n");
    printf("[6/6] Sistema listo.\n\n");

    printf("Bienvenido a ChronoOS.\n");

    return EXIT_SUCCESS;
}
