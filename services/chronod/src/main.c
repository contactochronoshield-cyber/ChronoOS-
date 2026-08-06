#include <stdio.h>
#include <unistd.h>
#include <signal.h>

volatile int running = 1;

void stop(int sig)
{
    running = 0;
}

int main()
{
    signal(SIGINT, stop);
    signal(SIGTERM, stop);

    printf("=========================================\n");
    printf("        ChronoD Service Manager\n");
    printf("          Version 1.0.0-alpha\n");
    printf("=========================================\n");

    printf("[ChronoD] Inicializando...\n");
    printf("[ChronoD] Servicios iniciados.\n");

    while(running)
    {
        sleep(1);
    }

    printf("[ChronoD] Cerrando servicios...\n");

    return 0;
}
