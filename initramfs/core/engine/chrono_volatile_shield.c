#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>

#define SECURE_BUFFER_SIZE 8192
static volatile char *protected_memory = NULL;

void emergency_purge(int sig) {
    if (protected_memory != NULL) {
        for (int i = 0; i < SECURE_BUFFER_SIZE; i++) {
            protected_memory[i] = (char)(i % 255);
        }
        munlock((const void *)protected_memory, SECURE_BUFFER_SIZE);
        free((void *)protected_memory);
        protected_memory = NULL;
    }
    _exit(0);
}

int main() {
    signal(SIGINT, emergency_purge);
    signal(SIGTERM, emergency_purge);
    protected_memory = (volatile char *)malloc(SECURE_BUFFER_SIZE);
    if (!protected_memory) return 1;
    if (mlock((const void *)protected_memory, SECURE_BUFFER_SIZE) == 0) {
        printf("[✓] Páginas de memoria bloqueadas en RAM física (Anti-Swap).\n");
    }
    while(1) { sleep(1); }
    return 0;
}
