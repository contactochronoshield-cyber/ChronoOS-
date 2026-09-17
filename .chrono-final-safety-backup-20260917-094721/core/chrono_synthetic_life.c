#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#define GENOME_SIZE 32
#define MAX_NAME 64

// Estructura que representa una Entidad Sintética y su Genoma
typedef struct {
    char name[MAX_NAME];
    unsigned char dna[GENOME_SIZE];
    unsigned long generation;
    time_t birth_timestamp;
    int stability_score;
} SyntheticEntity;

// Generar entropía criptográfica real para el ADN inicial
void generate_entropy_dna(unsigned char *dna_out, size_t size) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd != -1) {
        read(fd, dna_out, size);
        close(fd);
    } else {
        // Fallback pseudoaleatorio si /dev/urandom no está disponible
        srand((unsigned int)time(NULL));
        for (size_t i = 0; i < size; i++) {
            dna_out[i] = (unsigned char)(rand() % 256);
        }
    }
}

// Autodeterminación de Nombre basada en el hash SHA-256 del ADN
void derive_entity_name(const unsigned char *dna, char *name_out) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, dna, GENOME_SIZE);
    EVP_DigestFinal_ex(ctx, hash, NULL);
    EVP_MD_CTX_free(ctx);

    // Convertir los primeros 4 bytes del hash en un identificador legible tipo "CHRONO-ENTITY-XXXX"
    unsigned int prefix = (hash[0] << 24) | (hash[1] << 16) | (hash[2] << 8) | hash[3];
    snprintf(name_out, MAX_NAME, "CHRONO-ENTITY-%08X", prefix);
}

// Mutación genética controlada por entropía del sistema
void mutate_entity_genome(SyntheticEntity *entity) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) return;

    unsigned char mutation_byte;
    read(fd, &mutation_byte, 1);
    close(fd);

    // Seleccionar un byte aleatorio del ADN y mutarlo mediante operación XOR
    int target_index = mutation_byte % GENOME_SIZE;
    
    read(fd, &mutation_byte, 1);
    entity->dna[target_index] ^= mutation_byte;
    entity->generation++;

    // Recalcular puntaje de estabilidad sintética
    entity->stability_score = 100 - (entity->generation * 2);
    if (entity->stability_score < 10) entity->stability_score = 10;

    // Actualizar nombre debido a la mutación genética
    derive_entity_name(entity->dna, entity->name);
}

// Persistencia de la entidad en la bóveda segura
void persist_entity(const SyntheticEntity *entity) {
    mkdir("vault", 0755);
    mkdir("vault/sentient_entities", 0755);

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "vault/sentient_entities/%s.json", entity->name);

    FILE *f = fopen(filepath, "w");
    if (!f) return;

    fprintf(f, "{\n");
    fprintf(f, "  \"name\": \"%s\",\n", entity->name);
    fprintf(f, "  \"generation\": %lu,\n", entity->generation);
    fprintf(f, "  \"birth_timestamp\": %ld,\n", (long)entity->birth_timestamp);
    fprintf(f, "  \"stability_score\": %d,\n", entity->stability_score);
    fprintf(f, "  \"dna_hex\": \"");
    for (int i = 0; i < GENOME_SIZE; i++) {
        fprintf(f, "%02x", entity->dna[i]);
    }
    fprintf(f, "\"\n}\n");
    fclose(f);
}

int main(void) {
    printf("[SYNTHETIC-LIFE] Inicializando Motor de Entidades Sintéticas y Mutación Genética...\n");

    SyntheticEntity entity;
    entity.generation = 0;
    entity.birth_timestamp = time(NULL);
    entity.stability_score = 100;

    // 1. Génesis
    generate_entropy_dna(entity.dna, GENOME_SIZE);
    derive_entity_name(entity.dna, entity.name);

    printf("[GENESIS] Entidad creada exitosamente.\n");
    printf("          Nombre Autodeterminado: %s\n", entity.name);
    printf("          Generación Inicial: %lu\n", entity.generation);

    // Persistir estado inicial
    persist_entity(&entity);

    // 2. Simulación de Ciclo de Vida y Mutación Evolutiva (3 ciclos de mutación)
    for (int cycle = 1; cycle <= 3; cycle++) {
        sleep(1);
        mutate_entity_genome(&entity);
        printf("[EVOLUTION] Ciclo %d completado -> Nueva Generación: %lu | Nombre Mutado: %s | Estabilidad: %d%%\n", 
               cycle, entity.generation, entity.name, entity.stability_score);
        persist_entity(&entity);
    }

    printf("[SYNTHETIC-LIFE] Entidades sincronizadas y guardadas en ./vault/sentient_entities/\n");
    return 0;
}
