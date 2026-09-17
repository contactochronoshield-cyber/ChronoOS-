#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define WORLD_SIZE 32
#define MAX_ENTITIES 500
#define NUM_GENES 8
#define GEN_LOG "./var/logs/evolution_log.csv"

typedef struct {
    float metabolism_eff, move_speed, sensor_range, aggression;
    float repro_threshold, lifespan_gene, mutation_rate, social_gene;
} Genome;

typedef struct {
    Genome genes;
    float energy;
    int x, y, age, alive;
    unsigned long id, parent_id;
    int generation;
} Entity;

typedef struct { float resources[WORLD_SIZE][WORLD_SIZE]; } World;

static unsigned long next_id = 1;
float randf() { return (float)rand() / (float)RAND_MAX; }
float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

void world_init(World *w) {
    for (int i = 0; i < WORLD_SIZE; i++)
        for (int j = 0; j < WORLD_SIZE; j++)
            w->resources[i][j] = randf() * 0.5f;
}

void world_regenerate(World *w) {
    for (int i = 0; i < WORLD_SIZE; i++)
        for (int j = 0; j < WORLD_SIZE; j++)
            w->resources[i][j] = clampf(w->resources[i][j] + 0.01f, 0.0f, 1.0f);
}

Genome genome_random() {
    Genome g;
    g.metabolism_eff = randf(); g.move_speed = randf(); g.sensor_range = randf();
    g.aggression = randf(); g.repro_threshold = 0.5f + randf() * 0.5f;
    g.lifespan_gene = randf(); g.mutation_rate = 0.01f + randf() * 0.09f; g.social_gene = randf();
    return g;
}

Genome genome_mutate(Genome parent) {
    Genome g = parent;
    float *fields[NUM_GENES] = { &g.metabolism_eff, &g.move_speed, &g.sensor_range, &g.aggression,
        &g.repro_threshold, &g.lifespan_gene, &g.mutation_rate, &g.social_gene };
    for (int i = 0; i < NUM_GENES; i++)
        if (randf() < parent.mutation_rate)
            *fields[i] = clampf(*fields[i] + (randf() - 0.5f) * 0.3f, 0.0f, 1.0f);
    return g;
}

Genome genome_crossover(Genome a, Genome b) {
    Genome g;
    float *fa[NUM_GENES] = { &a.metabolism_eff, &a.move_speed, &a.sensor_range, &a.aggression,
        &a.repro_threshold, &a.lifespan_gene, &a.mutation_rate, &a.social_gene };
    float *fb[NUM_GENES] = { &b.metabolism_eff, &b.move_speed, &b.sensor_range, &b.aggression,
        &b.repro_threshold, &b.lifespan_gene, &b.mutation_rate, &b.social_gene };
    float *fg[NUM_GENES] = { &g.metabolism_eff, &g.move_speed, &g.sensor_range, &g.aggression,
        &g.repro_threshold, &g.lifespan_gene, &g.mutation_rate, &g.social_gene };
    for (int i = 0; i < NUM_GENES; i++) *fg[i] = (randf() < 0.5f) ? *fa[i] : *fb[i];
    return g;
}

void entity_spawn(Entity *e, Genome g, int x, int y, unsigned long parent_id, int gen) {
    e->genes = g; e->energy = 0.5f; e->x = x; e->y = y; e->age = 0; e->alive = 1;
    e->id = next_id++; e->parent_id = parent_id; e->generation = gen;
}

void entity_step(Entity *e, World *w) {
    if (!e->alive) return;
    int range = 1 + (int)(e->genes.sensor_range * 3);
    int best_x = e->x, best_y = e->y;
    float best_val = w->resources[e->x][e->y];
    for (int dx = -range; dx <= range; dx++)
        for (int dy = -range; dy <= range; dy++) {
            int nx = ((e->x + dx) % WORLD_SIZE + WORLD_SIZE) % WORLD_SIZE;
            int ny = ((e->y + dy) % WORLD_SIZE + WORLD_SIZE) % WORLD_SIZE;
            if (w->resources[nx][ny] > best_val) { best_val = w->resources[nx][ny]; best_x = nx; best_y = ny; }
        }
    float move_cost = 0.01f + e->genes.move_speed * 0.02f;
    if (best_x != e->x || best_y != e->y) { e->x = best_x; e->y = best_y; e->energy -= move_cost; }
    float consumed = w->resources[e->x][e->y] * e->genes.metabolism_eff;
    e->energy += consumed; w->resources[e->x][e->y] -= consumed;
    e->energy -= 0.015f; e->age++;
    int max_age = 50 + (int)(e->genes.lifespan_gene * 150);
    if (e->energy <= 0.0f || e->age > max_age) e->alive = 0;
}

int entity_can_reproduce(Entity *e) { return e->alive && e->energy >= e->genes.repro_threshold; }

typedef struct { int population; float avg_energy, avg_metabolism, genetic_diversity; int generation_max; } Metrics;

Metrics compute_metrics(Entity *pop, int n) {
    Metrics m = {0}; int alive_count = 0; float sum_energy = 0, sum_meta = 0, mean_genes[NUM_GENES] = {0};
    for (int i = 0; i < n; i++) {
        if (!pop[i].alive) continue;
        alive_count++; sum_energy += pop[i].energy; sum_meta += pop[i].genes.metabolism_eff;
        if (pop[i].generation > m.generation_max) m.generation_max = pop[i].generation;
        float *g[NUM_GENES] = { &pop[i].genes.metabolism_eff, &pop[i].genes.move_speed, &pop[i].genes.sensor_range,
            &pop[i].genes.aggression, &pop[i].genes.repro_threshold, &pop[i].genes.lifespan_gene,
            &pop[i].genes.mutation_rate, &pop[i].genes.social_gene };
        for (int k = 0; k < NUM_GENES; k++) mean_genes[k] += *g[k];
    }
    m.population = alive_count;
    if (alive_count == 0) return m;
    m.avg_energy = sum_energy / alive_count; m.avg_metabolism = sum_meta / alive_count;
    for (int k = 0; k < NUM_GENES; k++) mean_genes[k] /= alive_count;
    float var_sum = 0;
    for (int i = 0; i < n; i++) {
        if (!pop[i].alive) continue;
        float *g[NUM_GENES] = { &pop[i].genes.metabolism_eff, &pop[i].genes.move_speed, &pop[i].genes.sensor_range,
            &pop[i].genes.aggression, &pop[i].genes.repro_threshold, &pop[i].genes.lifespan_gene,
            &pop[i].genes.mutation_rate, &pop[i].genes.social_gene };
        for (int k = 0; k < NUM_GENES; k++) { float d = *g[k] - mean_genes[k]; var_sum += d * d; }
    }
    m.genetic_diversity = var_sum / (alive_count * NUM_GENES);
    return m;
}

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));
    int TICKS = (argc > 1) ? atoi(argv[1]) : 300;
    World world; world_init(&world);
    static Entity pop[MAX_ENTITIES];
    int pop_size = 40;
    for (int i = 0; i < pop_size; i++)
        entity_spawn(&pop[i], genome_random(), rand() % WORLD_SIZE, rand() % WORLD_SIZE, 0, 0);
    FILE *log = fopen(GEN_LOG, "w");
    if (log) fprintf(log, "tick,population,avg_energy,avg_metabolism,genetic_diversity,max_generation\n");
    printf("[chrono-synthetic-life] Iniciando simulacion: %d entidades, %d ticks, mundo %dx%d\n",
        pop_size, TICKS, WORLD_SIZE, WORLD_SIZE);
    for (int tick = 0; tick < TICKS; tick++) {
        world_regenerate(&world);
        for (int i = 0; i < pop_size; i++) entity_step(&pop[i], &world);
        int new_count = 0;
        Entity newborns[MAX_ENTITIES];
        for (int i = 0; i < pop_size && pop_size + new_count < MAX_ENTITIES; i++) {
            if (entity_can_reproduce(&pop[i])) {
                int partner = rand() % pop_size;
                Genome child_genome;
                if (pop[partner].alive && partner != i) child_genome = genome_crossover(pop[i].genes, pop[partner].genes);
                else child_genome = pop[i].genes;
                child_genome = genome_mutate(child_genome);
                entity_spawn(&newborns[new_count], child_genome, pop[i].x, pop[i].y, pop[i].id, pop[i].generation + 1);
                pop[i].energy *= 0.5f; new_count++;
            }
        }
        for (int i = 0; i < new_count && pop_size < MAX_ENTITIES; i++) pop[pop_size++] = newborns[i];
        int alive_idx = 0;
        for (int i = 0; i < pop_size; i++) if (pop[i].alive) pop[alive_idx++] = pop[i];
        pop_size = alive_idx;
        if (tick % 10 == 0 || tick == TICKS - 1) {
            Metrics m = compute_metrics(pop, pop_size);
            printf("[tick %4d] poblacion=%d energia_avg=%.3f metabolismo_avg=%.3f diversidad=%.4f gen_max=%d\n",
                tick, m.population, m.avg_energy, m.avg_metabolism, m.genetic_diversity, m.generation_max);
            if (log) fprintf(log, "%d,%d,%.4f,%.4f,%.4f,%d\n",
                tick, m.population, m.avg_energy, m.avg_metabolism, m.genetic_diversity, m.generation_max);
        }
        if (pop_size == 0) { printf("[!] Extincion total en tick %d\n", tick); break; }
    }
    if (log) fclose(log);
    printf("[✓] Simulacion terminada. Log de evolucion: %s\n", GEN_LOG);
    return 0;
}
