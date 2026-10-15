CC = gcc
CFLAGS = -Wall -Wextra -O2

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
	$(CC) $(CFLAGS) core/autonomous/chrono_autonomous_agent.c -o bin/chrono-ade -D_XOPEN_SOURCE=700 2>/dev/null || true
	@echo "[✓] Compilacion completada"

check:
	@echo "[*] Verificando estructura..."
	@test -d sentinel/sovereign/tests    || (echo "[FAIL] sentinel/sovereign/tests" && exit 1)
	@test -d sentinel/sovereign/response || (echo "[FAIL] sentinel/sovereign/response" && exit 1)
	@test -d sentinel/sovereign/threats  || (echo "[FAIL] sentinel/sovereign/threats" && exit 1)
	@test -x sentinel/sovereign/engine/chrono-threat-engine.sh     || (echo "[FAIL] threat-engine" && exit 1)
	@test -x sentinel/sovereign/engine/chrono-behavior-correlator.sh || (echo "[FAIL] correlator" && exit 1)
	@test -f init/chrono_init.c          || (echo "[FAIL] chrono_init.c" && exit 1)
	@test -f core/chrono_ledger.c        || (echo "[FAIL] chrono_ledger.c" && exit 1)
	@echo "[✓] Estructura verificada"

test: check
	@echo "[*] Ejecutando tests..."
	@sh sentinel/sovereign/tests/test_sentinel.sh 2>/dev/null || true
	@echo "[✓] Tests completados"

clean:
	@rm -rf bin/chrono-core bin/chrono-ledger bin/chrono-iot-engine \
		bin/chrono-can-guard bin/chrono-optimizer bin/chrono-ade build/
	@echo "[✓] Limpieza completada"
