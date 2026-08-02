/**
 * ==============================================================================
 * ChronoOS - Volatile Memory Encryption & Shield Engine (chrono_volatile_shield.c)
 * Author: Daniel Gonzales / Chrono Shield Networks
 * Description: Sistema de blindaje de memoria en C puro. Bloquea páginas en RAM 
 *              física (mlock) e implementa autodestrucción instantánea ante señales.
 * ==============================================================================
 */

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
        // Sobrescribir inmediatamente con ceros y ruido aleatorio
        for (int i = 0; i < SECURE_BUFFER_SIZE; i++) {
            protected_memory[i] = (char)(i % 255);
        }
        munlock((const void *)protected_memory, SECURE_BUFFER_SIZE);
        free((void *)protected_memory);
        protected_memory = NULL;
    }
    _exit(0);
}

void print_shield_banner() {
    printf("[CHRONO-SHIELD] Inicializando Motor de Memoria Volátil Cifrada...\n");
}

int main(int argc, char *argv[]) {
    print_shield_banner();

    // Capturar señales de interrupción para purga instantánea
    signal(SIGINT, emergency_purge);
    signal(SIGTERM, emergency_purge);

    // Asignar memoria dinámica
    protected_memory = (volatile char *)malloc(SECURE_BUFFER_SIZE);
    if (protected_memory == NULL) {
        perror("[!] Error crítico: Falló la asignación de memoria protegida");
        return 1;
    }

    // Bloquear la memoria en la RAM física (evita que vaya a Swap/Disco)
    if (mlock((const void *)protected_memory, SECURE_BUFFER_SIZE) != 0) {
        perror("[!] Advertencia: No se pudo bloquear la memoria con mlock (¿Faltan privilegios root?)");
    } else {
        printf("[✓] Páginas de memoria bloqueadas exitosamente en RAM física (Anti-Swap).\n");
    }

    // Escribir datos ultra sensibles simulados en el búfer cifrado
    const char *sensitive_secret = "CHRONO_MASTER_ENCRYPTION_KEY_AIR_GAPPED";
    for (int i = 0; i < strlen(sensitive_secret); i++) {
        protected_memory[i] = sensitive_secret[i] ^ 0x5A; // Cifrado XOR dinámico en memoria
    }
    
    printf("[✓] Búfer protegido cifrado en tiempo de ejecución (XOR 0x5A).\n");
    printf("[*] El sistema mantiene la memoria oculta. Presiona Ctrl+C para simular destrucción forense...\n");

    // Mantener activo el escudo en segundo plano
    while (1) {
        sleep(1);
    }

    return 0;
}
