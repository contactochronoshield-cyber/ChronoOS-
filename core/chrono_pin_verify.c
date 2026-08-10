/**
 * ChronoOS - Pin Verify
 * Verifica un bundle de Pin Forge: recalcula el Merkle root del directorio
 * ACTUAL y confirma la firma Ed25519 usando SOLO la llave publica.
 * Si algun archivo cambio (o se agrego/elimino), el root no coincide.
 * Si el bundle fue alterado, la firma no valida.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/pem.h>

#define MAX_PATH 2048
#define MAX_FILES 8192

typedef struct { char relpath[MAX_PATH]; unsigned char hash[SHA256_DIGEST_LENGTH]; } FileLeaf;
static FileLeaf leaves[MAX_FILES];
static int leaf_count = 0;

int compute_file_sha256(const char *filepath, unsigned char out[SHA256_DIGEST_LENGTH]) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    unsigned char buf[4096]; size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) EVP_DigestUpdate(ctx, buf, n);
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, out, &len);
    EVP_MD_CTX_free(ctx); fclose(f);
    return 0;
}

void walk_dir(const char *base, const char *rel) {
    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", base, rel);
    DIR *dir = opendir(fullpath);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (strcmp(entry->d_name, ".git") == 0) continue;
        if (strcmp(rel, "") == 0 && (strcmp(entry->d_name, "vault") == 0 || strcmp(entry->d_name, "build") == 0)) continue;
        char childrel[MAX_PATH];
        if (strlen(rel) == 0) snprintf(childrel, sizeof(childrel), "%s", entry->d_name);
        else snprintf(childrel, sizeof(childrel), "%s/%s", rel, entry->d_name);
        char childfull[MAX_PATH];
        snprintf(childfull, sizeof(childfull), "%s/%s", base, childrel);
        struct stat st;
        if (stat(childfull, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) walk_dir(base, childrel);
        else if (S_ISREG(st.st_mode)) {
            if (leaf_count >= MAX_FILES) continue;
            unsigned char h[SHA256_DIGEST_LENGTH];
            if (compute_file_sha256(childfull, h) == 0) {
                strncpy(leaves[leaf_count].relpath, childrel, MAX_PATH - 1);
                memcpy(leaves[leaf_count].hash, h, SHA256_DIGEST_LENGTH);
                leaf_count++;
            }
        }
    }
    closedir(dir);
}

int cmp_leaf(const void *a, const void *b) { return strcmp(((FileLeaf *)a)->relpath, ((FileLeaf *)b)->relpath); }

void leaf_hash(FileLeaf *leaf, unsigned char out[SHA256_DIGEST_LENGTH]) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, leaf->relpath, strlen(leaf->relpath));
    EVP_DigestUpdate(ctx, leaf->hash, SHA256_DIGEST_LENGTH);
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, out, &len);
    EVP_MD_CTX_free(ctx);
}

void compute_merkle_root(unsigned char root_out[SHA256_DIGEST_LENGTH]) {
    if (leaf_count == 0) { memset(root_out, 0, SHA256_DIGEST_LENGTH); return; }
    qsort(leaves, leaf_count, sizeof(FileLeaf), cmp_leaf);
    int n = leaf_count;
    unsigned char (*level)[SHA256_DIGEST_LENGTH] = malloc(n * SHA256_DIGEST_LENGTH);
    for (int i = 0; i < n; i++) leaf_hash(&leaves[i], level[i]);
    while (n > 1) {
        int next_n = (n + 1) / 2;
        unsigned char (*next_level)[SHA256_DIGEST_LENGTH] = malloc(next_n * SHA256_DIGEST_LENGTH);
        for (int i = 0; i < next_n; i++) {
            EVP_MD_CTX *ctx = EVP_MD_CTX_new();
            EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
            EVP_DigestUpdate(ctx, level[i * 2], SHA256_DIGEST_LENGTH);
            int right = (i * 2 + 1 < n) ? i * 2 + 1 : i * 2;
            EVP_DigestUpdate(ctx, level[right], SHA256_DIGEST_LENGTH);
            unsigned int len = 0;
            EVP_DigestFinal_ex(ctx, next_level[i], &len);
            EVP_MD_CTX_free(ctx);
        }
        free(level); level = next_level; n = next_n;
    }
    memcpy(root_out, level[0], SHA256_DIGEST_LENGTH);
    free(level);
}

void bytes_to_hex(unsigned char *bytes, int len, char *out) {
    for (int i = 0; i < len; i++) sprintf(out + i * 2, "%02x", bytes[i]);
    out[len * 2] = '\0';
}

int hex_to_bytes(const char *hex, unsigned char *out, int max_len) {
    int len = strlen(hex) / 2;
    if (len > max_len) return -1;
    for (int i = 0; i < len; i++) sscanf(hex + i * 2, "%2hhx", &out[i]);
    return len;
}

int main(int argc, char *argv[]) {
    if (argc < 3) { printf("Uso: chrono-pin-verify <directorio> <bundle.forge>\n"); return 1; }
    const char *target = argv[1];
    const char *bundle_path = argv[2];

    FILE *f = fopen(bundle_path, "r");
    if (!f) { perror("[!] No se pudo abrir el bundle"); return 1; }

    char line[512], stored_root[65] = {0}, sig_hex[257] = {0}, pubkey_file[512] = {0};
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strncmp(line, "MERKLE_ROOT:", 12) == 0) strncpy(stored_root, line + 12, 64);
        else if (strncmp(line, "SIGNATURE:", 10) == 0) strncpy(sig_hex, line + 10, 256);
        else if (strncmp(line, "PUBKEY_FILE:", 12) == 0) strncpy(pubkey_file, line + 12, 511);
    }
    fclose(f);

    if (!stored_root[0] || !sig_hex[0] || !pubkey_file[0]) {
        printf("[!] Bundle malformado o de version antigua (sin firma).\n");
        return 1;
    }

    printf("=== CHRONO PIN VERIFY ===\n");
    leaf_count = 0;
    walk_dir(target, "");
    unsigned char current_root[SHA256_DIGEST_LENGTH];
    compute_merkle_root(current_root);
    char current_root_hex[65];
    bytes_to_hex(current_root, SHA256_DIGEST_LENGTH, current_root_hex);

    printf("Merkle root actual:      %s\n", current_root_hex);
    printf("Merkle root del bundle:  %s\n", stored_root);

    int integrity_ok = (strcmp(current_root_hex, stored_root) == 0);
    printf("[%s] Integridad de archivos: %s\n", integrity_ok ? "OK" : "!!", integrity_ok ? "coincide" : "NO COINCIDE - archivos modificados");

    FILE *pf = fopen(pubkey_file, "r");
    if (!pf) { printf("[!] No se pudo abrir la llave publica: %s\n", pubkey_file); return 1; }
    EVP_PKEY *pkey = PEM_read_PUBKEY(pf, NULL, NULL, NULL);
    fclose(pf);
    if (!pkey) { printf("[!] Llave publica invalida\n"); return 1; }

    unsigned char sig[128];
    int sig_len = hex_to_bytes(sig_hex, sig, sizeof(sig));
    unsigned char stored_root_bytes[SHA256_DIGEST_LENGTH];
    hex_to_bytes(stored_root, stored_root_bytes, SHA256_DIGEST_LENGTH);

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_DigestVerifyInit(mdctx, NULL, NULL, NULL, pkey);
    int sig_ok = EVP_DigestVerify(mdctx, sig, sig_len, stored_root_bytes, SHA256_DIGEST_LENGTH) == 1;
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);

    printf("[%s] Firma digital: %s\n", sig_ok ? "OK" : "!!", sig_ok ? "valida (Ed25519)" : "INVALIDA o bundle alterado");

    printf("=========================\n");
    if (integrity_ok && sig_ok) {
        printf("RESULTADO: BUNDLE AUTENTICO Y VERIFICADO\n");
        return 0;
    } else {
        printf("RESULTADO: VERIFICACION FALLIDA - NO CONFIAR EN ESTE PAQUETE\n");
        return 1;
    }
}
