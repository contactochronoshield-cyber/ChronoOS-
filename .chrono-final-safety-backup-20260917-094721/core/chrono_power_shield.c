#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

// Perfiles de energía avanzados para cumplimiento IEC 62443 FR6
typedef enum {
    POWER_MODE_NORMAL = 0,
    POWER_MODE_CONSERVATION,
    POWER_MODE_DEEP_SLEEP_OFFGRID
} PowerMode;

volatile sig_atomic_t keep_running = 1;

void handle_signal(int sig) {
    (void)sig;
    keep_running = 0;
}

// Lectura segura de valores numéricos del kernel (sysfs)
int read_sysfs_int(const char *path, int default_val) {
    FILE *f = fopen(path, "r");
    if (!f) return default_val;
    int val = default_val;
    if (fscanf(f, "%d", &val) != 1) {
        val = default_val;
    }
    fclose(f);
    return val;
}

// Escritura de estado actual del sistema
void write_system_state(const char *state_str) {
    mkdir("var", 0755);
    mkdir("var/run", 0755);
    FILE *f = fopen("/var/run/chrono_power_state", "w");
    if (f) {
        fprintf(f, "%s\n", state_str);
        fclose(f);
    }
}

// Registro persistente en la caja negra de vuelo del sistema
void log_blackbox(const char *level, const char *msg) {
    mkdir("vault", 0755);
    mkdir("vault/carrington_safe", 0755);
    FILE *f = fopen("vault/carrington_safe/blackbox_flight.log", "a");
    if (f) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char time_buf[64];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", t);
        fprintf(f, "[%s] [%s] [DAEMON-POWER] %s\n", time_buf, level, msg);
        fclose(f);
    }
}

// Actualización del latido para prevenir el Dead Man's Switch
void update_heartbeat(void) {
    mkdir("config", 0755);
    FILE *f = fopen("config/.last_heartbeat", "w");
    if (f) {
        fprintf(f, "%ld\n", (long)time(NULL));
        fclose(f);
    }
}

// Ejecución de políticas de hardware reales según el modo
void execute_power_action(PowerMode mode) {
    if (mode == POWER_MODE_DEEP_SLEEP_OFFGRID) {
        log_blackbox("CRITICAL", "Aislamiento extremo: Apagando interfaces de red no esenciales.");
        system("ip link set wlan0 down 2>/dev/null || true");
        system("ip link set bluetooth down 2>/dev/null || true");
    } else if (mode == POWER_MODE_CONSERVATION) {
        log_blackbox("WARNING", "Modo Conservación: Limitando frecuencia de CPU a perfil eco.");
        system("for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do [ -w \"$gov\" ] && echo 'powersave' > \"$gov\" 2>/dev/null || true; done");
    } else {
        log_blackbox("INFO", "Restaurando rendimiento nominal del procesador.");
        system("for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do [ -w \"$gov\" ] && echo 'performance' > \"$gov\" 2>/dev/null || true; done");
    }
}

// Bucle principal del Daemon de Resiliencia Energética
void chrono_power_shield_daemon_loop(void) {
    log_blackbox("INFO", "Iniciando daemon autónomo de resiliencia y telemetría de energía.");
    
    while (keep_running) {
        // Detección real de fuentes de alimentación
        int grid_stable = read_sysfs_int("/sys/class/power_supply/AC/online", -1);
        if (grid_stable == -1) {
            grid_stable = read_sysfs_int("/sys/class/power_supply/ACAD/online", 1);
        }

        int batt_level = read_sysfs_int("/sys/class/power_supply/BAT0/capacity", 100);

        PowerMode current_mode = POWER_MODE_NORMAL;
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Estado Grid AC: %d | Nivel Batería: %d%%", grid_stable, batt_level);
        log_blackbox("DEBUG", log_msg);

        // Máquina de estados industrial
        if (grid_stable == 0 || batt_level < 20) {
            if (batt_level < 10) {
                current_mode = POWER_MODE_DEEP_SLEEP_OFFGRID;
                write_system_state("DEEP_SLEEP_OFFGRID");
            } else {
                current_mode = POWER_MODE_CONSERVATION;
                write_system_state("CONSERVATION");
            }
        } else {
            current_mode = POWER_MODE_NORMAL;
            write_system_state("NORMAL");
        }

        execute_power_action(current_mode);
        update_heartbeat();

        // Intervalo de sondeo de 5 segundos con comprobación de interrupción
        for (int i = 0; i < 5 && keep_running; i++) {
            sleep(1);
        }
    }
    
    log_blackbox("INFO", "Daemon de energía detenido de forma segura.");
}

int main(void) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    chrono_power_shield_daemon_loop();
    return 0;
}
