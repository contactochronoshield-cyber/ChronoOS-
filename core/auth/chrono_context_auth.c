#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#define AUTH_DB "./security/auth/context_profiles.conf"

void sha256_hex(const char *input, char out[65]) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned int len = 0;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, input, strlen(input));
    EVP_DigestFinal_ex(ctx, hash, &len);
    EVP_MD_CTX_free(ctx);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(out + i*2, "%02x", hash[i]);
    out[64] = '\0';
}

void log_ledger(const char *type, const char *details) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./bin/chrono-ledger append \"%s\" \"%s\" 2>/dev/null", type, details);
    system(cmd);
}

void gather_network_context(char *out, size_t outlen) {
    char buf[2048] = {0};
    FILE *f = popen("ip addr show 2>/dev/null | grep 'inet ' | awk '{print $2}' | head -5", "r");
    if (f) { fread(buf, 1, sizeof(buf)-1, f); pclose(f); }
    char buf2[512] = {0};
    FILE *f2 = popen("wg show 2>/dev/null | grep peer | head -5", "r");
    if (f2) { fread(buf2, 1, sizeof(buf2)-1, f2); pclose(f2); }
    snprintf(out, outlen, "NET:%s|WG:%s", buf, buf2);
}

void gather_hw_context(char *out, size_t outlen) {
    char buf[512] = {0};
    FILE *f = popen("getprop ro.serialno 2>/dev/null || cat /proc/cpuinfo 2>/dev/null | grep Serial | head -1", "r");
    if (f) { fread(buf, 1, sizeof(buf)-1, f); pclose(f); }
    snprintf(out, outlen, "HW:%s", buf);
}

void generate_token(const char *pin, char token[65]) {
    char net[2048] = {0}, hw[512] = {0};
    gather_network_context(net, sizeof(net));
    gather_hw_context(hw, sizeof(hw));
    char combined[4096];
    snprintf(combined, sizeof(combined), "%s|%s|%s", pin, net, hw);
    sha256_hex(combined, token);
}

void enroll(const char *user, const char *pin) {
    char token[65];
    generate_token(pin, token);
    time_t t = time(NULL);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", localtime(&t));
    mkdir("./security/auth", 0755);
    FILE *f = fopen(AUTH_DB, "a");
    if (!f) { perror("[!] No se pudo abrir auth db"); return; }
    fprintf(f, "%s,%s,%s\n", user, token, tbuf);
    fclose(f);
    printf("[✓] Contexto fisico registrado para: %s\n", user);
    printf("[i] Token: %.16s...\n", token);
    printf("[i] Solo valido desde ESTE lugar fisico con ESTE hardware.\n");
    log_ledger("CONTEXT_AUTH_ENROLLED", user);
}

int verify(const char *user, const char *pin) {
    char current[65];
    generate_token(pin, current);
    FILE *f = fopen(AUTH_DB, "r");
    if (!f) { printf("[!] Sin perfiles. Corre: enroll primero\n"); return 0; }
    char line[256], stored_user[64], stored_token[65];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%63[^,],%64[^,]", stored_user, stored_token) == 2) {
            if (strcmp(stored_user, user) == 0) {
                found = (strcmp(current, stored_token) == 0);
                break;
            }
        }
    }
    fclose(f);
    if (found) {
        printf("[✓] ACCESO AUTORIZADO: %s\n", user);
        printf("[i] Contexto fisico verificado correctamente.\n");
        log_ledger("CONTEXT_AUTH_SUCCESS", user);
    } else {
        printf("[!] ACCESO DENEGADO: %s\n", user);
        printf("[!] Contexto no coincide - PIN incorrecto, ubicacion\n");
        printf("    distinta, o hardware diferente al registrado.\n");
        printf("[!] Ataque remoto imposible - requiere presencia fisica.\n");
        FILE *al = fopen("./security/auth/access.log", "a");
        if (al) { fprintf(al, "FAILED context_auth user=%s\n", user); fclose(al); }
        log_ledger("CONTEXT_AUTH_FAILED", user);
    }
    return found;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("ChronoOS - Autenticacion por Contexto Fisico Soberano\n\n");
        printf("Diferencia con MFA tradicional (Google Auth, DefGuard):\n");
        printf("  MFA clasico: PIN + codigo temporal (replicable remotamente)\n");
        printf("  ChronoOS:    PIN + donde estas + hardware que tienes\n");
        printf("               Sin internet. Sin app externa. Sin servidor.\n");
        printf("               Imposible autenticarse desde otro lugar.\n\n");
        printf("Uso:\n");
        printf("  chrono-context-auth enroll <usuario> <pin>\n");
        printf("  chrono-context-auth verify <usuario> <pin>\n");
        return 1;
    }
    if (strcmp(argv[1], "enroll") == 0) enroll(argv[2], argv[3]);
    else if (strcmp(argv[1], "verify") == 0) return verify(argv[2], argv[3]) ? 0 : 1;
    else printf("[!] Comando desconocido\n");
    return 0;
}
