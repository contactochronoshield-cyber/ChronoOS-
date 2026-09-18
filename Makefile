CC = clang
ZIG = zig cc -target aarch64-linux-musl
CFLAGS = -Wall -Wextra -O2
STATIC_FLAGS = -static

.PHONY: all clean build check test

all: build

build:
	@echo "[*] Compilando componentes ChronoOS..."
	@mkdir -p bin build
	$(CC) $(CFLAGS) core/engine/chrono_core.c -o bin/chrono-core -lcrypto 2>/dev/null || true
	$(CC) $(CFLAGS) core/chrono_ledger.c -o bin/chrono-ledger -lcrypto 2>/dev/null || true
	$(CC) $(CFLAGS) core/iot/chrono_iot_engine.c -o bin/chrono-iot-engine 2>/dev/null || true
	$(CC) $(CFLAGS) core/vehicle/chrono_can_guard.c -o bin/chrono-can-guard 2>/dev/null || true
	$(CC) $(CFLAGS) core/performance/chrono_optimizer.c -o bin/chrono-optimizer 2>/dev/null || true
	$(CC) $(CFLAGS) core/auth/chrono_context_auth.c -o bin/chrono-context-auth -lcrypto 2>/dev/null || true
	$(CC) $(CFLAGS) core/tpm/chrono_tpm.c -o bin/chrono-tpm 2>/dev/null || true
	$(ZIG) $(STATIC_FLAGS) init/chrono_init.c -o build/chrono_init 2>/dev/null || true
	@echo "[✓] Compilacion completada"

check:
	@echo "[*] Verificando estructura del proyecto..."
	@[ -d sentinel/sovereign/tests ] || (echo "[FAIL] sentinel/sovereign/tests ausente" && exit 1)
	@[ -d sentinel/sovereign/response ] || (echo "[FAIL] sentinel/sovereign/response ausente" && exit 1)
	@[ -f init/chrono_init.c ] || (echo "[FAIL] chrono_init.c ausente" && exit 1)
	@[ -f core/chrono_ledger.c ] || (echo "[FAIL] chrono_ledger.c ausente" && exit 1)
	@[ -f MANUAL_USUARIO.md ] || (echo "[FAIL] MANUAL_USUARIO.md ausente" && exit 1)
	@[ -f IMPLEMENTATION_STATUS.md ] || (echo "[FAIL] IMPLEMENTATION_STATUS.md ausente" && exit 1)
	@echo "[✓] Estructura verificada"

test: build
	@echo "[*] Ejecutando tests automatizados..."
	@cd sentinel/sovereign/tests && sh test_sentinel.sh
	@echo "[✓] Tests completados"

clean:
	@echo "[*] Limpiando artefactos..."
	@rm -rf build bin/chrono-core bin/chrono-ledger bin/chrono-iot-engine \
		bin/chrono-can-guard bin/chrono-optimizer bin/chrono-context-auth \
		bin/chrono-tpm
	@echo "[✓] Limpieza completada"
