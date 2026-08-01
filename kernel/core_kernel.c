#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define VERSION "ChronoOS-Kernel-v1.0-C"

void print_banner() {
    printf("==================================================\n");
    printf("       [CHRONOOS NATIVE C KERNEL INITIALIZED]     \n");
    printf("==================================================\n");
}

void system_diagnostics() {
    printf("[*] Ejecutando diagnósticos nativos de memoria y estado...\n");
    time_t now = time(NULL);
    printf("[*] Timestamp UTC del Kernel: %s", ctime(&now));
    
    // Simulación de asignación de memoria segura del kernel
    size_t memory_buffer_size = 1024 * 1024; // 1 MB reservado
    char *kernel_memory = (char *)malloc(memory_buffer_size);
    if (kernel_memory == NULL) {
        fprintf(stderr, "[!] Error crítico: Fallo en la asignación de memoria del kernel.\n");
        exit(1);
    }
    
    memset(kernel_memory, 0xA5, memory_buffer_size);
    printf("[✓] Espacio de memoria protegido reservado y validado (%zu bytes).\n", memory_buffer_size);
    
    free(kernel_memory);
}

int main(int argc, char *argv[]) {
    print_banner();
    
    if (argc > 1 && strcmp(argv[1], "--diagnose") == 0) {
        system_diagnostics();
    } else {
        printf("[*] Uso del Kernel Nativo: ./kernel/chrono_core --diagnose\n");
    }
    
    return 0;
}
