/**
 * ChronoOS - Sovereign Personal Vault
 *
 * Guarda cualquier archivo cifrado con AES-256-GCM usando tu clave
 * personal. Nadie puede leer el contenido sin esa clave — ni CSN,
 * ni el servidor donde se guarda, ni nadie.
 *
 * Casos de uso:
 * - Usuario: guardar fotos, documentos, contraseñas si se daña el celular
 * - Empresa: backup cifrado de configs criticas si falla el servidor central
 *
 * El vault puede vivir en:
 * - Local (./vault/personal/)
 * - Sincronizado via chrono-airgap (sin internet, via QR)
 * - En cualquier servidor sin que el servidor pueda leer el contenido
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

#define VAULT_DIR     "./vault/personal"
#define VAULT_INDEX   "./vault/personal/.index"
#define KEY_LEN       32
#define IV_LEN        12
#define TAG_LEN       16
#define SALT_LEN      32

// Deriva una clave AES-256 desde una passphrase usando SHA-256 + salt
// Simple pero efectivo para este caso de uso
void derive_key(const char *passphrase, const unsigned char *salt,
                unsigned char *key_out) {
    unsigned char combined[512];
    size_t pp_len = strlen(passphrase);
    memcpy(combined, passphrase, pp_len);
    memcpy(combined + pp_len, salt, SALT_LEN);

    unsigned int len = 0;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, combined, pp_len + SALT_LEN);
    EVP_DigestFinal_ex(ctx, key_out, &len);
    EVP_MD_CTX_free(ctx);
}

int vault_store(const char *filepath, const char *passphrase,
                const char *label) {
    // Leer el archivo original
    FILE *fin = fopen(filepath, "rb");
    if (!fin) { perror("[!] No se pudo abrir el archivo"); return 1; }

    fseek(fin, 0, SEEK_END);
    long fsize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    unsigned char *plaintext = malloc(fsize);
    fread(plaintext, 1, fsize, fin);
    fclose(fin);

    // Generar salt, IV aleatorios
    unsigned char salt[SALT_LEN], iv[IV_LEN], tag[TAG_LEN];
    RAND_bytes(salt, SALT_LEN);
    RAND_bytes(iv, IV_LEN);

    // Derivar clave desde passphrase
    unsigned char key[KEY_LEN];
    derive_key(passphrase, salt, key);

    // Cifrar con AES-256-GCM
    unsigned char *ciphertext = malloc(fsize + EVP_MAX_BLOCK_LENGTH);
    int outlen = 0, total = 0;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &outlen, plaintext, fsize);
    total = outlen;
    EVP_EncryptFinal_ex(ctx, ciphertext + total, &outlen);
    total += outlen;
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag);
    EVP_CIPHER_CTX_free(ctx);

    // Generar nombre unico para el archivo en el vault
    char vault_filename[256];
    time_t t = time(NULL);
    snprintf(vault_filename, sizeof(vault_filename),
        "%s/%ld_%s.cvault", VAULT_DIR, t,
        label ? label : "item");

    // Guardar: salt + iv + tag + ciphertext
    FILE *fout = fopen(vault_filename, "wb");
    if (!fout) { perror("[!] No se pudo crear archivo en vault"); return 1; }
    fwrite(salt, 1, SALT_LEN, fout);
    fwrite(iv, 1, IV_LEN, fout);
    fwrite(tag, 1, TAG_LEN, fout);
    fwrite(ciphertext, 1, total, fout);
    fclose(fout);

    // Actualizar indice del vault
    FILE *idx = fopen(VAULT_INDEX, "a");
    if (idx) {
        char tbuf[32];
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", localtime(&t));
        fprintf(idx, "%s|%s|%s|%ld bytes\n",
            tbuf, label ? label : "sin_etiqueta",
            vault_filename, fsize);
        fclose(idx);
    }

    // Limpiar memoria sensible
    memset(key, 0, KEY_LEN);
    memset(plaintext, 0, fsize);
    free(plaintext);
    free(ciphertext);

    printf("[✓] Guardado en vault: %s\n", vault_filename);
    printf("[✓] Tamano original: %ld bytes | Cifrado: AES-256-GCM\n", fsize);
    printf("[i] Solo tu passphrase puede recuperar este archivo.\n");
    printf("[i] Ni CSN ni nadie mas puede leer el contenido.\n");
    return 0;
}

int vault_retrieve(const char *vault_file, const char *passphrase,
                   const char *output_path) {
    FILE *fin = fopen(vault_file, "rb");
    if (!fin) { perror("[!] Archivo del vault no encontrado"); return 1; }

    // Leer salt, iv, tag
    unsigned char salt[SALT_LEN], iv[IV_LEN], tag[TAG_LEN];
    fread(salt, 1, SALT_LEN, fin);
    fread(iv, 1, IV_LEN, fin);
    fread(tag, 1, TAG_LEN, fin);

    // Leer ciphertext
    fseek(fin, 0, SEEK_END);
    long total_size = ftell(fin);
    long cipher_size = total_size - SALT_LEN - IV_LEN - TAG_LEN;
    fseek(fin, SALT_LEN + IV_LEN + TAG_LEN, SEEK_SET);

    unsigned char *ciphertext = malloc(cipher_size);
    fread(ciphertext, 1, cipher_size, fin);
    fclose(fin);

    // Derivar clave
    unsigned char key[KEY_LEN];
    derive_key(passphrase, salt, key);

    // Descifrar
    unsigned char *plaintext = malloc(cipher_size + EVP_MAX_BLOCK_LENGTH);
    int outlen = 0, total = 0;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, NULL);
    EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &outlen, ciphertext, cipher_size);
    total = outlen;
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, tag);
    int ret = EVP_DecryptFinal_ex(ctx, plaintext + total, &outlen);
    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) {
        printf("[!] PASSPHRASE INCORRECTA o archivo corrupto.\n");
        printf("[!] No se pudo recuperar el contenido.\n");
        free(ciphertext);
        free(plaintext);
        return 1;
    }
    total += outlen;

    // Guardar archivo recuperado
    FILE *fout = fopen(output_path, "wb");
    if (!fout) { perror("[!] No se pudo crear archivo de salida"); return 1; }
    fwrite(plaintext, 1, total, fout);
    fclose(fout);

    // Limpiar memoria
    memset(key, 0, KEY_LEN);
    memset(plaintext, 0, total);
    free(ciphertext);
    free(plaintext);

    printf("[✓] Archivo recuperado: %s (%d bytes)\n", output_path, total);
    return 0;
}

void vault_list() {
    FILE *idx = fopen(VAULT_INDEX, "r");
    if (!idx) {
        printf("[i] El vault esta vacio.\n");
        return;
    }
    printf("============================================\n");
    printf(" VAULT PERSONAL SOBERANO - Contenido\n");
    printf("============================================\n");
    printf("%-20s %-20s %s\n", "FECHA", "ETIQUETA", "TAMANO");
    printf("--------------------------------------------\n");
    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), idx)) {
        char date[32], label[64], path[256], size[32];
        if (sscanf(line, "%31[^|]|%63[^|]|%255[^|]|%31[^\n]",
                date, label, path, size) >= 3) {
            printf("%-20s %-20s %s\n", date, label, size);
            count++;
        }
    }
    fclose(idx);
    printf("--------------------------------------------\n");
    printf("Total: %d item(s) en el vault\n", count);
    printf("[i] Usa 'retrieve' con tu passphrase para recuperar.\n");
}

int main(int argc, char *argv[]) {
    mkdir(VAULT_DIR, 0700); // Solo el propietario puede leer

    if (argc < 2) {
        printf("ChronoOS - Vault Personal Soberano\n\n");
        printf("Guarda archivos cifrados que solo TU puedes recuperar.\n");
        printf("Sin passphrase correcta, nadie puede leer el contenido.\n\n");
        printf("Uso:\n");
        printf("  chrono-vault store <archivo> <passphrase> [etiqueta]\n");
        printf("  chrono-vault retrieve <archivo.cvault> <passphrase> <destino>\n");
        printf("  chrono-vault list\n\n");
        printf("Ejemplos:\n");
        printf("  chrono-vault store fotos.zip \"mi clave secreta\" Fotos2026\n");
        printf("  chrono-vault store config.json \"mi clave\" BackupServidor\n");
        printf("  chrono-vault list\n");
        printf("  chrono-vault retrieve vault/personal/123_Fotos2026.cvault \\\n");
        printf("               \"mi clave secreta\" fotos_recuperadas.zip\n");
        return 1;
    }

    if (strcmp(argv[1], "store") == 0 && argc >= 4) {
        return vault_store(argv[2], argv[3],
                          argc >= 5 ? argv[4] : NULL);
    } else if (strcmp(argv[1], "retrieve") == 0 && argc >= 5) {
        return vault_retrieve(argv[2], argv[3], argv[4]);
    } else if (strcmp(argv[1], "list") == 0) {
        vault_list();
    } else {
        printf("[!] Argumentos invalidos\n");
        return 1;
    }
    return 0;
}
