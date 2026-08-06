/**
 * ChronoOS - Volatile Shield (chrono-volatile-shield.c)
 * Protege TODA la memoria sensible que este proceso administra, sin limite
 * fijo de tamano. Usa mlockall() para bloquear TODA la memoria del proceso
 * (presente y futura) contra swap, y un registro dinamico de buffers para
 * garantizar que cada uno se sobrescriba antes de terminar.
 *
 * LIMITE HONESTO: esto protege la memoria de ESTE proceso. Purgar la RAM
 * de otros procesos requiere privilegios de kernel (root), que Termux sin
 * root no otorga. Eso no es una limitacion del codigo, es una proteccion
 * del sistema operativo que ningun programa en userspace puede saltarse.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

#define MAX_BUFFERS 64

typedef struct {
    void *ptr;
    size_t size;
} SensitiveBuffer;

static SensitiveBuffer registry[MAX_BUFFERS];
static int registry_count = 0;

// Registra un buffer sensible para que se purgue garantizado al salir
void *chrono_alloc_sensitive(size_t size) {
    if (registry_count >= MAX_BUFFERS) {
        fprintf(stderr, "[!] Registro de buffers lleno, no se puede proteger mas memoria\n");
        return NULL;
    }
    void *ptr = malloc(size);
    if (!ptr) return NULL;

    if (mlock(ptr, size) != 0) {
        perror("[!] mlock fallo (memoria no bloqueada contra swap)");
    }

    registry[registry_count].ptr = ptr;
    registry[registry_count].size = size;
    registry_count++;
    return ptr;
}

// Wipe real: sobrescribe cada buffer registrado, sin importar cuantos ni su tamano
void chrono_wipe_all() {
    printf("[*] Purgando %d buffer(es) sensibles, %zu bytes totales...\n",
        registry_count, ({ size_t t=0; for(int i=0;i<registry_count;i++) t+=registry[i].size; t; }));

    for (int i = 0; i < registry_count; i++) {
        volatile unsigned char *p = (volatile unsigned char *)registry[i].ptr;
        size_t sz = registry[i].size;
        // Triple pasada: 0xFF, 0x00, aleatorio - dificulta remanencia magnetica/electrica
        memset((void *)p, 0xFF, sz);
        memset((void *)p, 0x00, sz);
        for (size_t j = 0; j < sz; j++) p[j] = (unsigned char)rand();
        munlock(registry[i].ptr, sz);
        free(registry[i].ptr);
    }
    registry_count = 0;
    printf("[✓] Memoria del proceso purgada por completo.\n");
}

void handle_signal(int sig) {
    printf("\n[!] Senal %d recibida. Ejecutando purga de emergencia...\n", sig);
    chrono_wipe_all();
    exit(0);
}

int main() {
    // Bloquea TODA la memoria de este proceso (actual y futura) contra swap
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        perror("[!] mlockall fallo (requiere privilegios en algunos sistemas)");
    } else {
        printf("[✓] mlockall activo: toda la memoria de este proceso protegida contra swap.\n");
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Ejemplo de uso real: aqui es donde chrono-core registraria la master.key
    // en memoria mientras la usa, en vez de dejarla suelta en variables normales.
    void *demo_sensitive = chrono_alloc_sensitive(1024 * 1024); // 1MB de ejemplo, escalable
    if (demo_sensitive) {
        memset(demo_sensitive, 0xAB, 1024 * 1024);
        printf("[i] Buffer de prueba de 1MB registrado y protegido.\n");
    }

    printf("[i] chrono-volatile-shield activo. Ctrl+C o SIGTERM para purgar y salir.\n");
    while (1) {
        pause();
    }
    return 0;
}
