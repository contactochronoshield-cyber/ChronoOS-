CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -I$(ROOT)/core

ROOT := $(CURDIR)
BUILD_DIR := $(ROOT)/build
BIN_DIR := $(ROOT)/bin

.PHONY: all build check test clean

all: build

build:
	@echo "[*] Compilando ChronoOS..."
	@mkdir -p "$(BUILD_DIR)" "$(BIN_DIR)"

	@test -f core/chrono_ledger.c || { echo "[FAIL] core/chrono_ledger.c ausente"; exit 1; }
	$(CC) $(CFLAGS) core/chrono_ledger.c -o "$(BIN_DIR)/chrono-ledger" -lcrypto

	@test -f core/iot/chrono_iot_engine.c || { echo "[FAIL] core/iot/chrono_iot_engine.c ausente"; exit 1; }
	$(CC) $(CFLAGS) core/iot/chrono_iot_engine.c core/common/chrono_shell_guard.c -o "$(BIN_DIR)/chrono-iot-engine"

	@test -f core/vehicle/chrono_can_guard.c || { echo "[FAIL] core/vehicle/chrono_can_guard.c ausente"; exit 1; }
	$(CC) $(CFLAGS) core/vehicle/chrono_can_guard.c core/common/chrono_exec.c -o "$(BIN_DIR)/chrono-can-guard"

	@test -f core/engine/chrono_core.c || { echo "[FAIL] core/engine/chrono_core.c ausente"; exit 1; }
	$(CC) $(CFLAGS) core/engine/chrono_core.c -o "$(BIN_DIR)/chrono-core" -lcrypto

	@if [ -f core/auth/chrono_context_auth.c ]; then \
		$(CC) $(CFLAGS) core/auth/chrono_context_auth.c core/common/chrono_shell_guard.c -o "$(BIN_DIR)/chrono-context-auth" -lcrypto; \
	else \
		echo "[INFO] chrono_context_auth.c no presente; omitido"; \
	fi

	@if [ -f core/tpm/chrono_tpm.c ]; then \
		$(CC) $(CFLAGS) core/tpm/chrono_tpm.c core/common/chrono_shell_guard.c -o "$(BIN_DIR)/chrono-tpm"; \
	else \
		echo "[INFO] chrono_tpm.c no presente; omitido"; \
	fi

	@if [ -f core/chrono_power_shield.c ]; then \
		$(CC) $(CFLAGS) core/chrono_power_shield.c core/common/chrono_shell_guard.c -o "$(BUILD_DIR)/chrono_power_shield"; \
	fi

	@if [ -f core/chrono_panic_protocol.c ]; then \
		$(CC) $(CFLAGS) core/chrono_panic_protocol.c core/common/chrono_shell_guard.c -o "$(BUILD_DIR)/chrono_panic_protocol"; \
	fi

	@if [ -f core/chrono_pin_forge.c ]; then \
		$(CC) $(CFLAGS) core/chrono_pin_forge.c -o "$(BUILD_DIR)/chrono_pin_forge" -lcrypto; \
	fi

	@if [ -f core/chrono_master_orchestrator.c ]; then \
		$(CC) $(CFLAGS) core/chrono_master_orchestrator.c core/common/chrono_shell_guard.c -o "$(BUILD_DIR)/chrono_master"; \
	fi

	@if [ -f core/chrono_synthetic_life.c ]; then \
		$(CC) $(CFLAGS) core/chrono_synthetic_life.c -o "$(BUILD_DIR)/chrono_synthetic_life" -lcrypto; \
	fi

	@echo "[OK] Compilacion completada"

check:
	@echo "[*] Verificando estructura..."

	@test -d sentinel/sovereign/tests || { echo "[FAIL] sentinel/sovereign/tests"; exit 1; }
	@test -d sentinel/sovereign/response || { echo "[FAIL] sentinel/sovereign/response"; exit 1; }
	@test -f IMPLEMENTATION_STATUS.md || { echo "[FAIL] IMPLEMENTATION_STATUS.md"; exit 1; }

	@test -f core/chrono_ledger.c || { echo "[FAIL] core/chrono_ledger.c"; exit 1; }
	@test -f core/iot/chrono_iot_engine.c || { echo "[FAIL] core/iot/chrono_iot_engine.c"; exit 1; }
	@test -f core/vehicle/chrono_can_guard.c || { echo "[FAIL] core/vehicle/chrono_can_guard.c"; exit 1; }

	@test -f sentinel/sovereign/tests/test_sentinel.sh || { echo "[FAIL] test_sentinel.sh"; exit 1; }
	@test -f sentinel/sovereign/response/threat_response.sh || { echo "[FAIL] threat_response.sh"; exit 1; }

	@echo "[OK] Estructura verificada"

test: check build
	@echo "[*] Ejecutando tests..."
	@cd "$(ROOT)" && sh sentinel/sovereign/tests/test_sentinel.sh

clean:
	@echo "[*] Limpiando artefactos..."
	@rm -rf "$(BUILD_DIR)"
	@rm -f "$(BIN_DIR)/chrono-core"
	@rm -f "$(BIN_DIR)/chrono-ledger"
	@rm -f "$(BIN_DIR)/chrono-iot-engine"
	@rm -f "$(BIN_DIR)/chrono-can-guard"
	@rm -f "$(BIN_DIR)/chrono-context-auth"
	@rm -f "$(BIN_DIR)/chrono-tpm"
	@echo "[OK] Limpieza completada"
