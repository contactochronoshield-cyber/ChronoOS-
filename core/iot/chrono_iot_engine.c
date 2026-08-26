#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#define SENSOR_WHITELIST "./etc/chrono/iot_sensors.conf"
#define ALARM_RULES "./etc/chrono/iot_alarm_rules.conf"
#define BUFFER_PATH "./var/db/iot_buffer.csv"
#define MAX_SENSORS 256
#define MAX_RULES 128

typedef struct {
    char sensor_id[64];
    char sensor_type[32];
    char location[128];
    int active;
} Sensor;

typedef struct {
    char sensor_id[64];
    char metric[32];
    float threshold_min;
    float threshold_max;
    char alarm_level[16];
    char action[64];
} AlarmRule;

static Sensor sensors[MAX_SENSORS];
static int sensor_count = 0;
static AlarmRule rules[MAX_RULES];
static int rule_count = 0;

void log_ledger(const char *event_type, const char *details) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./bin/chrono-ledger append \"%s\" \"%s\" 2>/dev/null", event_type, details);
    system(cmd);
}

void load_sensors() {
    FILE *f = fopen(SENSOR_WHITELIST, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f) && sensor_count < MAX_SENSORS) {
        if (line[0] == '#' || line[0] == '\n') continue;
        Sensor s;
        if (sscanf(line, "%63[^,],%31[^,],%127[^\n]", s.sensor_id, s.sensor_type, s.location) >= 3) {
            s.active = 1;
            sensors[sensor_count++] = s;
        }
    }
    fclose(f);
    printf("[✓] %d sensores autorizados cargados\n", sensor_count);
}

void load_rules() {
    FILE *f = fopen(ALARM_RULES, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f) && rule_count < MAX_RULES) {
        if (line[0] == '#' || line[0] == '\n') continue;
        AlarmRule r;
        if (sscanf(line, "%63[^,],%31[^,],%f,%f,%15[^,],%63[^\n]",
                r.sensor_id, r.metric, &r.threshold_min, &r.threshold_max,
                r.alarm_level, r.action) >= 6) {
            rules[rule_count++] = r;
        }
    }
    fclose(f);
    printf("[✓] %d reglas de alarma cargadas\n", rule_count);
}

int is_authorized(const char *sensor_id) {
    for (int i = 0; i < sensor_count; i++)
        if (strcmp(sensors[i].sensor_id, sensor_id) == 0) return 1;
    return 0;
}

void buffer_reading(const char *sensor_id, const char *metric, float value, const char *ts) {
    FILE *f = fopen(BUFFER_PATH, "a");
    if (!f) return;
    fprintf(f, "%s,%s,%s,%.4f,PENDING\n", ts, sensor_id, metric, value);
    fclose(f);
}

void check_alarms(const char *sensor_id, const char *metric, float value) {
    for (int i = 0; i < rule_count; i++) {
        if ((strcmp(rules[i].sensor_id, sensor_id) == 0 || strcmp(rules[i].sensor_id, "*") == 0)
            && strcmp(rules[i].metric, metric) == 0) {
            if (value < rules[i].threshold_min || value > rules[i].threshold_max) {
                printf("\n  ╔══════════════════════════════════════╗\n");
                printf("  ║ ALARMA %-8s                      ║\n", rules[i].alarm_level);
                printf("  ║ Sensor:  %-28s ║\n", sensor_id);
                printf("  ║ Metrica: %-10s Valor: %-8.2f ║\n", metric, value);
                printf("  ║ Rango seguro: [%.1f - %.1f]          ║\n", rules[i].threshold_min, rules[i].threshold_max);
                printf("  ╚══════════════════════════════════════╝\n");
                char details[512];
                snprintf(details, sizeof(details), "sensor=%s metrica=%s valor=%.2f nivel=%s", sensor_id, metric, value, rules[i].alarm_level);
                log_ledger("IOT_ALARM", details);
                if (strcmp(rules[i].action, "panic") == 0)
                    printf("  [!!!] CONDICION CRITICA - EVACUAR ZONA\n");
            }
        }
    }
}

void process_reading(const char *sensor_id, const char *metric, float value) {
    if (!is_authorized(sensor_id)) {
        printf("[!] SENSOR NO AUTORIZADO: %s\n", sensor_id);
        log_ledger("IOT_UNAUTHORIZED_SENSOR", sensor_id);
        return;
    }
    time_t t = time(NULL);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", localtime(&t));
    printf("  [IoT] %s | %s | %s = %.2f\n", tbuf, sensor_id, metric, value);
    buffer_reading(sensor_id, metric, value, tbuf);
    check_alarms(sensor_id, metric, value);
    char details[256];
    snprintf(details, sizeof(details), "sensor=%s metrica=%s valor=%.2f", sensor_id, metric, value);
    log_ledger("IOT_READING", details);
}

void show_status() {
    printf("=== ChronoOS IoT Engine Status ===\n");
    printf("Sensores autorizados: %d\n", sensor_count);
    printf("Reglas de alarma:     %d\n", rule_count);
    FILE *f = fopen(BUFFER_PATH, "r");
    int pending = 0;
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f))
            if (strstr(line, "PENDING")) pending++;
        fclose(f);
    }
    printf("Lecturas en buffer:   %d\n", pending);
}

int main(int argc, char *argv[]) {
    mkdir("./var/db", 0755);
    load_sensors();
    load_rules();
    if (argc < 2) {
        printf("Uso: chrono-iot-engine [process <id> <metrica> <valor>|status]\n");
        return 1;
    }
    if (strcmp(argv[1], "process") == 0 && argc >= 5)
        process_reading(argv[2], argv[3], atof(argv[4]));
    else if (strcmp(argv[1], "status") == 0)
        show_status();
    return 0;
}
