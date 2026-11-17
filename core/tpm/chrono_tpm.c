#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>

#define TPM_PARENT "0x81000001"
#define TPM_DIR "./var/lib/chrono/tpm"
#define MASTER_KEY "./security/auth/master.key"

static int run_cmd(char *const args[]) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("[TPM] fork");
        return -1;
    }

    if (pid == 0) {
        execvp(args[0], args);
        _exit(127);
    }

    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return -1;
    }

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static void log_ledger(const char *event, const char *details) {
    char *args[] = {
        "./bin/chrono-ledger",
        "append",
        (char *)event,
        (char *)details,
        NULL
    };

    if (run_cmd(args) != 0)
        fprintf(stderr, "[TPM] ADVERTENCIA: no se pudo registrar en Ledger\n");
}

static int tpm_available(void) {
    char *args[] = {"tpm2_getrandom", "1", "--hex", NULL};
    return run_cmd(args) == 0;
}

static int check_parent(void) {
    char *args[] = {
        "tpm2_readpublic",
        "-c", TPM_PARENT,
        NULL
    };

    return run_cmd(args) == 0;
}

static int valid_master_key(void) {
    struct stat st;

    if (stat(MASTER_KEY, &st) != 0) {
        fprintf(stderr, "[TPM] No existe master.key\n");
        return 0;
    }

    if (!S_ISREG(st.st_mode) || st.st_size != 32) {
        fprintf(stderr, "[TPM] master.key debe ser un archivo regular de 32 bytes\n");
        return 0;
    }

    if ((st.st_mode & 077) != 0) {
        fprintf(stderr, "[TPM] Permisos inseguros en master.key. Se requiere 600.\n");
        return 0;
    }

    return 1;
}

static int prepare_directory(void) {
    if (mkdir(TPM_DIR, 0700) != 0 && errno != EEXIST) {
        perror("[TPM] mkdir");
        return 0;
    }

    struct stat st;

    if (stat(TPM_DIR, &st) != 0 ||
        !S_ISDIR(st.st_mode) ||
        (st.st_mode & 077) != 0) {
        fprintf(stderr, "[TPM] Directorio TPM inseguro\n");
        return 0;
    }

    return 1;
}

static void tpm_get_random(int bytes) {
    if (bytes < 1 || bytes > 64) {
        fprintf(stderr, "[TPM] Cantidad permitida: 1-64 bytes\n");
        return;
    }

    if (!tpm_available()) {
        fprintf(stderr, "[TPM] Hardware no disponible. No se declara entropia TPM.\n");
        log_ledger("TPM_RANDOM_FAILED", "hardware_unavailable");
        return;
    }

    char amount[16];
    snprintf(amount, sizeof(amount), "%d", bytes);

    char *args[] = {"tpm2_getrandom", amount, "--hex", NULL};

    printf("[TPM] Solicitando %d bytes al hardware...\n", bytes);

    if (run_cmd(args) == 0) {
        printf("\n[TPM] Entropia recibida del TPM.\n");
        log_ledger("TPM_RANDOM_GENERATED", "source=hardware_tpm2");
    } else {
        fprintf(stderr, "[TPM] ERROR obteniendo entropia\n");
        log_ledger("TPM_RANDOM_FAILED", "tpm_command_failed");
    }
}

static void tpm_seal_master_key(void) {
    printf("[TPM] Preparando sellado ligado a PCR 0,1,7...\n");

    if (!tpm_available() || !check_parent()) {
        fprintf(stderr, "[TPM] TPM o clave padre no disponible.\n");
        log_ledger("TPM_SEAL_FAILED", "hardware_or_parent_unavailable");
        return;
    }

    if (!valid_master_key() || !prepare_directory()) {
        log_ledger("TPM_SEAL_FAILED", "master_key_or_directory_invalid");
        return;
    }

    char *policy[] = {
        "tpm2_createpolicy",
        "--policy-pcr",
        "-l", "sha256:0,1,7",
        "-L", TPM_DIR "/policy.bin",
        NULL
    };

    if (run_cmd(policy) != 0) {
        fprintf(stderr, "[TPM] Fallo creando politica PCR.\n");
        log_ledger("TPM_SEAL_FAILED", "policy_creation_failed");
        return;
    }

    char *create[] = {
        "tpm2_create",
        "-C", TPM_PARENT,
        "-i", MASTER_KEY,
        "-u", TPM_DIR "/sealed.pub",
        "-r", TPM_DIR "/sealed.priv",
        "-L", TPM_DIR "/policy.bin",
        NULL
    };

    if (run_cmd(create) != 0) {
        fprintf(stderr, "[TPM] Fallo creando objeto sellado.\n");
        log_ledger("TPM_SEAL_FAILED", "object_creation_failed");
        return;
    }

    struct stat pub, priv, policy_file;

    if (stat(TPM_DIR "/sealed.pub", &pub) != 0 ||
        stat(TPM_DIR "/sealed.priv", &priv) != 0 ||
        stat(TPM_DIR "/policy.bin", &policy_file) != 0 ||
        pub.st_size == 0 || priv.st_size == 0 ||
        policy_file.st_size == 0) {
        fprintf(stderr, "[TPM] Artefactos incompletos.\n");
        log_ledger("TPM_SEAL_FAILED", "artifact_validation_failed");
        return;
    }

    chmod(TPM_DIR "/sealed.pub", 0600);
    chmod(TPM_DIR "/sealed.priv", 0600);
    chmod(TPM_DIR "/policy.bin", 0600);

    printf("[TPM] Objetos sellados creados y verificados.\n");
    printf("[TPM] Todavia se requiere probar load/unseal y recuperacion.\n");

    log_ledger("TPM_SEALED_OBJECT_CREATED", "pcr=sha256:0,1,7");
}

static void tpm_measure(void) {
    printf("[TPM] Midiendo PCR actuales...\n");

    if (!tpm_available()) {
        fprintf(stderr, "[TPM] Hardware no disponible.\n");
        log_ledger("TPM_MEASURE_FAILED", "hardware_unavailable");
        return;
    }

    char *args[] = {
        "tpm2_pcrread",
        "sha256:0,1,7",
        NULL
    };

    if (run_cmd(args) != 0) {
        fprintf(stderr, "[TPM] No se pudieron leer los PCR.\n");
        log_ledger("TPM_MEASURE_FAILED", "pcr_read_failed");
        return;
    }

    log_ledger("TPM_PCR_READ", "bank=sha256 pcr=0,1,7");
}

static void tpm_attest(void) {
    fprintf(stderr, "[TPM] Atestacion remota bloqueada.\n");
    fprintf(stderr, "[TPM] Falta provisionar y verificar una AK (Attestation Key).\n");
    fprintf(stderr, "[TPM] No se generara una prueba ficticia.\n");

    log_ledger("TPM_ATTESTATION_UNAVAILABLE", "attestation_key_not_provisioned");
}

static void tpm_panic(void) {
    fprintf(stderr, "[TPM] Operacion destructiva bloqueada.\n");
    fprintf(stderr, "[TPM] No se eliminaran objetos persistentes sin protocolo de recuperacion.\n");

    log_ledger("TPM_PANIC_BLOCKED", "destructive_operation_requires_recovery_protocol");
}

int main(int argc, char *argv[]) {
    printf("=== ChronoOS TPM 2.0 Integration ===\n");

    int available = tpm_available();

    printf("Hardware TPM: %s\n\n",
           available ? "DISPONIBLE" : "NO DISPONIBLE");

    if (argc < 2) {
        fprintf(stderr,
            "Uso:\n"
            " chrono-tpm random <bytes>\n"
            " chrono-tpm seal\n"
            " chrono-tpm measure\n"
            " chrono-tpm attest\n"
            " chrono-tpm panic\n");
        return 2;
    }

    if (strcmp(argv[1], "random") == 0) {
        char *end = NULL;
        long n = argc > 2 ? strtol(argv[2], &end, 10) : 32;

        if ((argc > 2 && (!end || *end != '\0')) ||
            n < 1 || n > 64) {
            fprintf(stderr, "[TPM] Argumento random invalido.\n");
            return 2;
        }

        tpm_get_random((int)n);
        return 0;
    }

    if (strcmp(argv[1], "seal") == 0)
        tpm_seal_master_key();
    else if (strcmp(argv[1], "measure") == 0)
        tpm_measure();
    else if (strcmp(argv[1], "attest") == 0)
        tpm_attest();
    else if (strcmp(argv[1], "panic") == 0)
        tpm_panic();
    else {
        fprintf(stderr, "[TPM] Comando desconocido.\n");
        return 2;
    }

    return 0;
}
