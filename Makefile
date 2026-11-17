CPPFLAGS += -Icore
CC = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all clean build check test security integration

all: test

build:
	@echo "[CHRONOOS] Compilando componentes..."
	@mkdir -p bin build
	$(CC) $(CPPFLAGS) $(CFLAGS) core/engine/chrono_core.c -o bin/chrono-core -lcrypto
	$(CC) $(CPPFLAGS) $(CFLAGS) core/chrono_ledger.c -o bin/chrono-ledger -lcrypto
	$(CC) $(CPPFLAGS) $(CFLAGS) core/iot/chrono_iot_engine.c core/common/chrono_shell_guard.c -o bin/chrono-iot-engine
	$(CC) $(CPPFLAGS) $(CFLAGS) core/vehicle/chrono_can_guard.c -o bin/chrono-can-guard
	$(CC) $(CPPFLAGS) $(CFLAGS) core/autonomous/chrono_autonomous_agent.c -o bin/chrono-ade -D_XOPEN_SOURCE=700
	$(CC) $(CPPFLAGS) $(CFLAGS) core/auth/chrono_context_auth.c core/common/chrono_shell_guard.c -o bin/chrono-context-auth -lcrypto
	$(CC) $(CPPFLAGS) $(CFLAGS) core/tpm/chrono_tpm.c core/common/chrono_shell_guard.c -o bin/chrono-tpm
	@echo "[CHRONOOS] Compilacion completada"

check:
	@echo "[CHRONOOS] Verificando estructura..."
	@test -d sentinel/sovereign/tests
	@test -d sentinel/sovereign/response
	@test -d sentinel/sovereign/threats
	@test -x sentinel/sovereign/engine/chrono-threat-engine.sh
	@test -x sentinel/sovereign/engine/chrono-behavior-correlator.sh
	@test -f init/chrono_init.c
	@test -f core/chrono_ledger.c
	@test -f core/identity/chrono_device_identity.c
	@test -f security/ledger/ledger_writer.py
	@test -f security/policy/destructive-operations.conf
	@test -f security/pki/root-of-trust-policy.md
	@echo "[CHRONOOS] Estructura verificada"

security:
	@bash tests/security/test-security-layout.sh
	@bash tests/integration/security-baseline.sh

integration:
	@echo "[CHRONOOS] Ejecutando pruebas de integracion disponibles..."
	@bash tests/integration/security-baseline.sh

test: check build security
	@echo "[CHRONOOS] Ejecutando Sentinel..."
	@sh sentinel/sovereign/tests/test_sentinel.sh
	@echo "[CHRONOOS] Suite completada"

clean:
	@rm -f bin/chrono-core bin/chrono-ledger bin/chrono-iot-engine bin/chrono-can-guard bin/chrono-ade bin/chrono-context-auth bin/chrono-tpm
	@rm -rf build/
