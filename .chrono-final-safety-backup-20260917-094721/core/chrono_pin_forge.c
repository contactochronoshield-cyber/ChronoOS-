/**
 * ChronoOS - Pin Forge v5
 * Arbol Merkle REAL (recursivo, orden deterministico, hojas+ramas) y
 * firma digital Ed25519 real del root - no solo un hash etiquetado como
 * "verificado". Genera par de llaves si no existe, firma el bundle con
 * la llave privada, el verificador usa solo la llave publica.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/pem.h>

#define MAX_PATH 2048
#define MAX_FILES 8192

typedef struct {
    char relpath[MAX_PATH];
    unsigned char hash[SHA256_DIGEST_LENGTH];
} FileLeaf;

static FileLeaf leaves[MAX_FILES];
static int leaf_count = 0;

int compute_file_sha256(const char *filepath, unsigned char out[SHA256_DIGEST_LENGTH]) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    unsigned char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) EVP_DigestUpdate(ctx, buf, n);
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, out, &len);
    EVP_MD_CTX_free(ctx);
    fclose(f);
    return 0;
}

// Recorrido RECURSIVO real, ignora .git, vault, build (artefactos, no fuente)
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

        if (S_ISDIR(st.st_mode)) {
            walk_dir(base, childrel);
        } else if (S_ISREG(st.st_mode)) {
            if (leaf_count >= MAX_FILES) { fprintf(stderr, "[!] Limite de archivos alcanzado\n"); continue; }
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

int cmp_leaf(const void *a, const void *b) {
    return strcmp(((FileLeaf *)a)->relpath, ((FileLeaf *)b)->relpath);
}

// Hash de hoja real: sha256(relpath || file_hash) - orden deterministico por path
void leaf_hash(FileLeaf *leaf, unsigned char out[SHA256_DIGEST_LENGTH]) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, leaf->relpath, strlen(leaf->relpath));
    EVP_DigestUpdate(ctx, leaf->hash, SHA256_DIGEST_LENGTH);
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, out, &len);
    EVP_MD_CTX_free(ctx);
}

// Arbol Merkle REAL: empareja hojas y sube nivel por nivel hasta la raiz.
// Si un nivel tiene cantidad impar, el ultimo nodo se duplica (estandar).
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
            int right = (i * 2 + 1 < n) ? i * 2 + 1 : i * 2; // duplica el ultimo si es impar
            EVP_DigestUpdate(ctx, level[right], SHA256_DIGEST_LENGTH);
            unsigned int len = 0;
            EVP_DigestFinal_ex(ctx, next_level[i], &len);
            EVP_MD_CTX_free(ctx);
        }
        free(level);
        level = next_level;
        n = next_n;
    }
    memcpy(root_out, level[0], SHA256_DIGEST_LENGTH);
    free(level);
}

void bytes_to_hex(unsigned char *bytes, int len, char *out) {
    for (int i = 0; i < len; i++) sprintf(out + i * 2, "%02x", bytes[i]);
    out[len * 2] = '\0';
}

int ensure_keypair(const char *priv_path, const char *pub_path) {
    if (access(priv_path, F_OK) == 0 && access(pub_path, F_OK) == 0) return 0;

    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
    EVP_PKEY_keygen_init(pctx);
    EVP_PKEY *pkey = NULL;
    if (EVP_PKEY_keygen(pctx, &pkey) <= 0) { EVP_PKEY_CTX_free(pctx); return -1; }

    FILE *fpriv = fopen(priv_path, "w");
    PEM_write_PrivateKey(fpriv, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(fpriv);
    chmod(priv_path, 0600);

    FILE *fpub = fopen(pub_path, "w");
    PEM_write_PUBKEY(fpub, pkey);
    fclose(fpub);

    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(pctx);
    printf("[PIN-FORGE] Nuevo par de llaves Ed25519 generado: %s / %s\n", priv_path, pub_path);
    return 0;
}

int sign_root(const char *priv_path, unsigned char *root, int root_len, unsigned char *sig_out, size_t *sig_len) {
    FILE *f = fopen(priv_path, "r");
    if (!f) return -1;
    EVP_PKEY *pkey = PEM_read_PrivateKey(f, NULL, NULL, NULL);
    fclose(f);
    if (!pkey) return -1;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_DigestSignInit(mdctx, NULL, NULL, NULL, pkey);
    EVP_DigestSign(mdctx, sig_out, sig_len, root, root_len);
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);
    return 0;
}

void forge_bundle(const char *project_dir, const char *output_bundle_path) {
    printf("[PIN-FORGE] Construyendo bundle soberano (Merkle real + firma Ed25519) para: %s\n", project_dir);

    leaf_count = 0;
    walk_dir(project_dir, "");
    printf("[PIN-FORGE] %d archivos indexados (recursivo).\n", leaf_count);

    unsigned char root[SHA256_DIGEST_LENGTH];
    compute_merkle_root(root);
    char root_hex[65];
    bytes_to_hex(root, SHA256_DIGEST_LENGTH, root_hex);
    printf("[PIN-FORGE] Merkle Root (real, recursivo, ordenado): %s\n", root_hex);

    mkdir("security/auth", 0755);
    const char *priv_path = "security/auth/pinforge_ed25519.pem";
    const char *pub_path = "security/auth/pinforge_ed25519.pub";
    if (ensure_keypair(priv_path, pub_path) != 0) {
        fprintf(stderr, "[!] No se pudo generar/leer el par de llaves\n");
        return;
    }

    unsigned char sig[128];
    size_t sig_len = sizeof(sig);
    if (sign_root(priv_path, root, SHA256_DIGEST_LENGTH, sig, &sig_len) != 0) {
        fprintf(stderr, "[!] No se pudo firmar el Merkle root\n");
        return;
    }
    char sig_hex[257];
    bytes_to_hex(sig, sig_len, sig_hex);

    FILE *bundle = fopen(output_bundle_path, "w");
    if (!bundle) { perror("[ERROR] No se pudo crear el bundle"); return; }
    fprintf(bundle, "CHRONO_PIN_FORGE_BUNDLE_V5\n");
    fprintf(bundle, "TARGET_DIR:%s\n", project_dir);
    fprintf(bundle, "FILE_COUNT:%d\n", leaf_count);
    fprintf(bundle, "MERKLE_ROOT:%s\n", root_hex);
    fprintf(bundle, "SIGNATURE_ALGO:Ed25519\n");
    fprintf(bundle, "SIGNATURE:%s\n", sig_hex);
    fprintf(bundle, "PUBKEY_FILE:%s\n", pub_path);
    fclose(bundle);

    printf("[PIN-FORGE] Bundle firmado generado en: %s\n", output_bundle_path);
    printf("[PIN-FORGE] Verificar con: chrono-pin-verify %s %s\n", project_dir, output_bundle_path);
}

int main(int argc, char *argv[]) {
    const char *target = (argc > 1) ? argv[1] : ".";
    const char *output = (argc > 2) ? argv[2] : "./vault/chrono_package.forge";
    mkdir("vault", 0755);
    forge_bundle(target, output);
    return 0;
}
