/**
 * ChronoOS - Sovereign C Kernel Core Supervisor (chrono_core.c)
 * Cifrado real AES-256-GCM para bovedas (crypto-shredding real).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#define CHRONO_VERSION "10.1-SOVEREIGN"
#define KEY_LEN 32
#define IV_LEN 12
#define TAG_LEN 16

void print_banner() { printf("\n  CHRONO-OS NATIVE C KERNEL CORE v%s\n\n", CHRONO_VERSION); }

int read_key(const char *keypath, unsigned char *key) {
    FILE *f = fopen(keypath, "rb");
    if (!f) { perror("[!] No se pudo abrir la llave"); return 0; }
    size_t r = fread(key, 1, KEY_LEN, f);
    fclose(f);
    return r == KEY_LEN;
}

int encrypt_file(const char *in_path, const char *out_path, const char *keypath) {
    unsigned char key[KEY_LEN], iv[IV_LEN], tag[TAG_LEN];
    if (!read_key(keypath, key)) return 1;
    if (RAND_bytes(iv, IV_LEN) != 1) { fprintf(stderr, "[!] Fallo generando IV\n"); return 1; }
    FILE *fin = fopen(in_path, "rb");
    if (!fin) { perror("[!] No se pudo abrir origen"); return 1; }
    FILE *fout = fopen(out_path, "wb");
    if (!fout) { perror("[!] No se pudo crear destino"); fclose(fin); return 1; }
    unsigned char inbuf[4096], outbuf[4096 + EVP_MAX_BLOCK_LENGTH];
    int outlen, total = 0;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);
    fwrite(iv, 1, IV_LEN, fout);
    size_t n;
    while ((n = fread(inbuf, 1, sizeof(inbuf), fin)) > 0) {
        EVP_EncryptUpdate(ctx, outbuf, &outlen, inbuf, n);
        fwrite(outbuf, 1, outlen, fout);
        total += outlen;
    }
    EVP_EncryptFinal_ex(ctx, outbuf, &outlen);
    fwrite(outbuf, 1, outlen, fout);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag);
    fwrite(tag, 1, TAG_LEN, fout);
    EVP_CIPHER_CTX_free(ctx);
    fclose(fin); fclose(fout);
    printf("[✓] Cifrado: %s -> %s (%d bytes)\n", in_path, out_path, total);
    return 0;
}

int decrypt_file(const char *in_path, const char *out_path, const char *keypath) {
    unsigned char key[KEY_LEN], iv[IV_LEN], tag[TAG_LEN];
    if (!read_key(keypath, key)) return 1;
    FILE *fin = fopen(in_path, "rb");
    if (!fin) { perror("[!] No se pudo abrir cifrado"); return 1; }
    fseek(fin, 0, SEEK_END);
    long fsize = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    long ciphertext_len = fsize - IV_LEN - TAG_LEN;
    if (ciphertext_len < 0) { fprintf(stderr, "[!] Archivo corrupto\n"); fclose(fin); return 1; }
    fread(iv, 1, IV_LEN, fin);
    unsigned char *cbuf = malloc(ciphertext_len);
    fread(cbuf, 1, ciphertext_len, fin);
    fread(tag, 1, TAG_LEN, fin);
    fclose(fin);
    FILE *fout = fopen(out_path, "wb");
    if (!fout) { perror("[!] No se pudo crear salida"); free(cbuf); return 1; }
    unsigned char *outbuf = malloc(ciphertext_len + EVP_MAX_BLOCK_LENGTH);
    int outlen, total = 0;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, NULL);
    EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv);
    EVP_DecryptUpdate(ctx, outbuf, &outlen, cbuf, ciphertext_len);
    total += outlen;
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, tag);
    int ret = EVP_DecryptFinal_ex(ctx, outbuf + total, &outlen);
    EVP_CIPHER_CTX_free(ctx);
    if (ret <= 0) {
        fprintf(stderr, "[!] FALLO DE VERIFICACION: llave incorrecta o archivo alterado.\n");
        fclose(fout); free(cbuf); free(outbuf); remove(out_path);
        return 1;
    }
    total += outlen;
    fwrite(outbuf, 1, total, fout);
    fclose(fout); free(cbuf); free(outbuf);
    printf("[✓] Descifrado: %s -> %s (%d bytes)\n", in_path, out_path, total);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_banner();
        printf("Uso: chrono-core [status | encrypt <in> <out> <keyfile> | decrypt <in> <out> <keyfile>]\n");
        return 1;
    }
    if (strcmp(argv[1], "status") == 0) {
        print_banner();
        struct sysinfo si;
        if (sysinfo(&si) == 0)
            printf("[STATUS] RAM Total: %lu MB | Libre: %lu MB | Tareas: %d\n",
                si.totalram / 1024 / 1024, si.freeram / 1024 / 1024, si.procs);
    } else if (strcmp(argv[1], "encrypt") == 0) {
        if (argc < 5) { fprintf(stderr, "Uso: chrono-core encrypt <in> <out> <keyfile>\n"); return 1; }
        return encrypt_file(argv[2], argv[3], argv[4]);
    } else if (strcmp(argv[1], "decrypt") == 0) {
        if (argc < 5) { fprintf(stderr, "Uso: chrono-core decrypt <in> <out> <keyfile>\n"); return 1; }
        return decrypt_file(argv[2], argv[3], argv[4]);
    } else {
        printf("[!] Comando desconocido: %s\n", argv[1]);
        return 1;
    }
    return 0;
}
