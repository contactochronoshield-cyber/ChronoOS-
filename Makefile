CC ?= gcc

ROOT := $(CURDIR)
BUILD_DIR := $(ROOT)/build
BIN_DIR := $(ROOT)/bin

CFLAGS ?= -O2 -Wall -Wextra -I$(ROOT)/core

.PHONY: all build check test clean antenna isp cumbia assurance cnem

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

	@test -f core/antenna/chrono_antenna.c || { echo "[FAIL] core/antenna/chrono_antenna.c ausente"; exit 1; }
	$(CC) $(CFLAGS) core/antenna/chrono_antenna.c -o "$(BIN_DIR)/chrono-antenna"

	@test -f core/isp/chrono_isp.c || { echo "[FAIL] core/isp/chrono_isp.c ausente"; exit 1; }
	$(CC) $(CFLAGS) core/isp/chrono_isp.c -o "$(BIN_DIR)/chrono-isp"

	@test -f core/cumbia/chrono_cumbia.c || { echo "[FAIL] core/cumbia/chrono_cumbia.c ausente"; exit 1; }
	$(CC) $(CFLAGS) core/cumbia/chrono_cumbia.c -o "$(BIN_DIR)/chrono-cumbia"

	@test -f core/assurance/chrono_assurance.c || { echo "[FAIL] core/assurance/chrono_assurance.c ausente"; exit 1; }
	@test -f core/assurance/test_chrono_assurance.c || { echo "[FAIL] core/assurance/test_chrono_assurance.c ausente"; exit 1; }
	$(CC) $(CFLAGS) core/assurance/chrono_assurance.c core/assurance/test_chrono_assurance.c -o "$(BIN_DIR)/chrono-assurance"

	@test -f core/cnem/chrono_cnem.c || { echo "[FAIL] core/cnem/chrono_cnem.c ausente"; exit 1; }
	@test -f core/cnem/test_chrono_cnem.c || { echo "[FAIL] core/cnem/test_chrono_cnem.c ausente"; exit 1; }
	$(CC) $(CFLAGS) core/cnem/chrono_cnem.c core/cnem/test_chrono_cnem.c -o "$(BIN_DIR)/chrono-cnem"

	@echo "[OK] Compilacion completada"

check:
	@echo "[*] Verificando estructura..."

	@test -d sentinel/sovereign/tests || { echo "[FAIL] sentinel/sovereign/tests"; exit 1; }
	@test -d sentinel/sovereign/response || { echo "[FAIL] sentinel/sovereign/response"; exit 1; }
	@test -f IMPLEMENTATION_STATUS.md || { echo "[FAIL] IMPLEMENTATION_STATUS.md"; exit 1; }

	@test -f core/chrono_ledger.c || { echo "[FAIL] core/chrono_ledger.c"; exit 1; }
	@test -f core/iot/chrono_iot_engine.c || { echo "[FAIL] core/iot/chrono_iot_engine.c"; exit 1; }
	@test -f core/vehicle/chrono_can_guard.c || { echo "[FAIL] core/vehicle/chrono_can_guard.c"; exit 1; }

	@test -f core/antenna/chrono_antenna.c || { echo "[FAIL] core/antenna/chrono_antenna.c"; exit 1; }
	@test -f core/antenna/chrono_antenna.h || { echo "[FAIL] core/antenna/chrono_antenna.h"; exit 1; }
	@test -x core/antenna/test_chrono_antenna.sh || { echo "[FAIL] test_chrono_antenna.sh"; exit 1; }

	@test -f core/isp/chrono_isp.c || { echo "[FAIL] core/isp/chrono_isp.c"; exit 1; }
	@test -f core/isp/chrono_isp.h || { echo "[FAIL] core/isp/chrono_isp.h"; exit 1; }
	@test -x core/isp/test_chrono_isp.sh || { echo "[FAIL] test_chrono_isp.sh"; exit 1; }

	@test -f core/cumbia/chrono_cumbia.c || { echo "[FAIL] core/cumbia/chrono_cumbia.c"; exit 1; }
	@test -f core/cumbia/chrono_cumbia.h || { echo "[FAIL] core/cumbia/chrono_cumbia.h"; exit 1; }
	@test -x core/cumbia/test_chrono_cumbia.sh || { echo "[FAIL] test_chrono_cumbia.sh"; exit 1; }

	@test -f core/assurance/chrono_assurance.c || { echo "[FAIL] core/assurance/chrono_assurance.c"; exit 1; }
	@test -f core/assurance/chrono_assurance.h || { echo "[FAIL] core/assurance/chrono_assurance.h"; exit 1; }
	@test -f core/assurance/test_chrono_assurance.c || { echo "[FAIL] test_chrono_assurance.c"; exit 1; }
	@test -x core/assurance/test_chrono_assurance.sh || { echo "[FAIL] test_chrono_assurance.sh"; exit 1; }

	@test -f core/cnem/chrono_cnem.c || { echo "[FAIL] core/cnem/chrono_cnem.c"; exit 1; }
	@test -f core/cnem/chrono_cnem.h || { echo "[FAIL] core/cnem/chrono_cnem.h"; exit 1; }
	@test -f core/cnem/test_chrono_cnem.c || { echo "[FAIL] test_chrono_cnem.c"; exit 1; }
	@test -x core/cnem/test_chrono_cnem.sh || { echo "[FAIL] test_chrono_cnem.sh"; exit 1; }

	@test -f sentinel/sovereign/tests/test_sentinel.sh || { echo "[FAIL] test_sentinel.sh"; exit 1; }
	@test -f sentinel/sovereign/response/threat_response.sh || { echo "[FAIL] threat_response.sh"; exit 1; }

	@echo "[OK] Estructura verificada"

test: check build
	@echo "[*] Ejecutando tests..."
	@cd "$(ROOT)" && bash sentinel/sovereign/tests/test_sentinel.sh
	@cd "$(ROOT)" && bash core/antenna/test_chrono_antenna.sh
	@cd "$(ROOT)" && bash core/isp/test_chrono_isp.sh
	@cd "$(ROOT)" && bash core/cumbia/test_chrono_cumbia.sh
	@cd "$(ROOT)" && bash core/assurance/test_chrono_assurance.sh
	@cd "$(ROOT)" && bash core/cnem/test_chrono_cnem.sh
	@echo "[OK] Todos los tests completados"

antenna: build
	@echo "[*] Ejecutando Chrono Antenna Intelligence..."
	@./bin/chrono-antenna --self-test
	@echo "[OK] Chrono Antenna Intelligence operativo"

isp: build
	@echo "[*] Ejecutando Chrono ISP Intelligence..."
	@./bin/chrono-isp --self-test
	@echo "[OK] Chrono ISP Intelligence operativo"

cumbia: build
	@echo "[*] Ejecutando Cumbia..."
	@./bin/chrono-cumbia --self-test
	@echo "[OK] Cumbia operativo"

assurance: build
	@echo "[*] Ejecutando Chrono Assurance Engine..."
	@./bin/chrono-assurance --self-test
	@echo "[OK] Chrono Assurance Engine operativo"


cnem: build
	@echo "[*] Ejecutando Chrono Network Event Model..."
	@./bin/chrono-cnem --self-test
	@echo "[OK] Chrono Network Event Model operativo"
clean:
	@echo "[*] Limpiando artefactos..."
	@rm -rf "$(BUILD_DIR)"
	@rm -f "$(BIN_DIR)/chrono-core"
	@rm -f "$(BIN_DIR)/chrono-ledger"
	@rm -f "$(BIN_DIR)/chrono-iot-engine"
	@rm -f "$(BIN_DIR)/chrono-can-guard"
	@rm -f "$(BIN_DIR)/chrono-context-auth"
	@rm -f "$(BIN_DIR)/chrono-tpm"
	@rm -f "$(BIN_DIR)/chrono-antenna"
	@rm -f "$(BIN_DIR)/chrono-isp"
	@rm -f "$(BIN_DIR)/chrono-cumbia"
	@rm -f "$(BIN_DIR)/chrono-assurance"
	@rm -f "$(BIN_DIR)/chrono-cnem"
	@echo "[OK] Limpieza completada"
