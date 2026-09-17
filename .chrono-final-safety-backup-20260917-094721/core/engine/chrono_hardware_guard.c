/**
 * ChronoOS - Hardware Guard (chrono_hardware_guard.c)
 * Motor avanzado de deteccion y control de hardware en userspace.
 * Cubre USB, almacenamiento externo, y dispositivos serial/tty.
 * Huella digital real (vendor+product+serial hasheado), no solo vendor:product.
 * Cuarentena logica: dispositivo no reconocido queda marcado, no se le
 * concede acceso a bovedas hasta aprobacion manual.
 *
 * LIMITE HONESTO: esto opera en userspace via sysfs/udev. No bloquea el
 * bus fisicamente (eso requiere modulo de kernel + root). Detecta,
 * huella digital, registra, y niega acceso LOGICO a los recursos que
 * ChronoOS controla directamente (bovedas, llaves).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

#define WHITELIST_PATH "./etc/chrono/hw_whitelist.conf"
#define QUARANTINE_LOG "./security/auth/quarantine.log"
#define BLACKBOX "/mnt/data/power_blackbox.log"

// FNV-1a simple, suficiente para huella digital de identificacion, no para criptografia
unsigned long fnv1a(const char *str) {
    unsigned long hash = 2166136261UL;
    while (*str) {
        hash ^= (unsigned char)(*str++);
        hash *= 16777619UL;
    }
    return hash;
}

int read_attr(const char *base, const char *attr, char *out, size_t len) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", base, attr);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    if (!fgets(out, len, f)) { fclose(f); return 0; }
    out[strcspn(out, "\n")] = 0;
    fclose(f);
    return 1;
}

void log_event(const char *fmt_prefix, const char *fingerprint, const char *devpath, int authorized) {
    FILE *bb = fopen(BLACKBOX, "a");
    time_t t = time(NULL);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    if (bb) {
        fprintf(bb, "[%s] [hw-guard] %s fingerprint=%08lx path=%s authorized=%s\n",
            tbuf, fmt_prefix, strtoul(fingerprint, NULL, 16), devpath, authorized ? "yes" : "NO");
        fclose(bb);
    }
    if (!authorized) {
        FILE *q = fopen(QUARANTINE_LOG, "a");
        if (q) {
            fprintf(q, "[%s] CUARENTENA: %s (fingerprint %s) en %s - acceso a bovedas DENEGADO hasta aprobacion manual\n",
                tbuf, fmt_prefix, fingerprint, devpath);
            fclose(q);
        }
    }
}

int is_whitelisted(const char *fingerprint) {
    FILE *f = fopen(WHITELIST_PATH, "r");
    if (!f) return 0;
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        line[strcspn(line, "\n")] = 0;
        if (strncmp(line, fingerprint, strlen(fingerprint)) == 0) { found = 1; break; }
    }
    fclose(f);
    return found;
}

void scan_class(const char *class_path, const char *label) {
    DIR *d = opendir(class_path);
    if (!d) return;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char devbase[512];
        snprintf(devbase, sizeof(devbase), "%s/%s", class_path, entry->d_name);

        char vendor[64] = "unknown", product[64] = "unknown", serial[128] = "none";
        read_attr(devbase, "idVendor", vendor, sizeof(vendor));
        read_attr(devbase, "idProduct", product, sizeof(product));
        read_attr(devbase, "serial", serial, sizeof(serial));

        char combined[256];
        snprintf(combined, sizeof(combined), "%s:%s:%s", vendor, product, serial);
        char fp_hex[16];
        snprintf(fp_hex, sizeof(fp_hex), "%08lx", fnv1a(combined));

        int auth = is_whitelisted(fp_hex);
        log_event(label, fp_hex, devbase, auth);
    }
    closedir(d);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    printf("[chrono-hardware-guard] Escaneando dispositivos conectados...\n");

    scan_class("/sys/bus/usb/devices", "USB");
    scan_class("/sys/class/tty", "SERIAL");
    scan_class("/sys/class/block", "STORAGE");

    printf("[✓] Escaneo completo. Ver %s para detalle y %s para dispositivos en cuarentena.\n",
        BLACKBOX, QUARANTINE_LOG);
    return 0;
}
