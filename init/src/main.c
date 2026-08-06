#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define VERSION_FILE "../VERSION"
#define CONFIG_FILE "../config/chrono.conf"

static void banner(void)
{
    printf("\n");
    printf("=================================================\n");
    printf("               ChronoOS Boot Manager\n");
    printf("                 Version 1.0.0-alpha\n");
    printf("=================================================\n\n");
}

static void load_version(void)
{
    FILE *fp = fopen(VERSION_FILE, "r");

    if (!fp)
    {
        printf("[ERROR] VERSION file not found\n");
        return;
    }

    char version[64];

    if (fgets(version, sizeof(version), fp))
        printf("[ OK ] Version: %s", version);

    fclose(fp);
}

static void load_config(void)
{
    FILE *fp = fopen(CONFIG_FILE, "r");

    if (!fp)
    {
        printf("[ERROR] chrono.conf not found\n");
        return;
    }

    printf("[ OK ] Configuration loaded\n");

    fclose(fp);
}

static void prepare_runtime(void)
{
    mkdir("/tmp",0755);
    mkdir("/run",0755);

    printf("[ OK ] Runtime ready\n");
}

static void start_chronod(void)
{
    pid_t pid=fork();

    if(pid==0)
    {
        execl("../services/chronod/chronod",
              "chronod",
              NULL);

        perror("execl");
        exit(EXIT_FAILURE);
    }

    if(pid>0)
    {
        printf("[ OK ] ChronoD PID %d\n",pid);
        return;
    }

    perror("fork");
}

int main(void)
{
    banner();

    printf("[1/5] Loading version...\n");
    load_version();

    printf("[2/5] Loading configuration...\n");
    load_config();

    printf("[3/5] Preparing runtime...\n");
    prepare_runtime();

    printf("[4/5] Starting ChronoD...\n");
    start_chronod();

    printf("[5/5] Boot completed.\n\n");

    while(1)
    {
        int status;

        pid_t pid=wait(&status);

        if(pid>0)
        {
            printf("[INIT] Service %d finished.\n",pid);
            printf("[INIT] Restarting ChronoD...\n");

            start_chronod();
        }
    }

    return 0;
}
