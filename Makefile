CC = gcc
CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -lcrypto

BINARIES = \
	build/chrono_can_guard \
	build/chrono_power_shield \
	build/chrono_panic_protocol \
	build/chrono_pin_forge \
	build/chrono_master \
	build/chrono_synthetic_life

.PHONY: all directories build check test clean

all: build

directories:
	mkdir -p build core scripts vault/carrington_safe vault/sentient_entities crypto_keys config var/run

build: directories $(BINARIES)

build/chrono_can_guard: core/vehicle/chrono_can_guard.c core/common/chrono_exec.c core/common/chrono_exec.h
	$(CC) $(CFLAGS) core/vehicle/chrono_can_guard.c core/common/chrono_exec.c -o $@

build/chrono_power_shield: core/chrono_power_shield.c core/common/chrono_shell_guard.c core/common/chrono_shell_guard.h
	$(CC) $(CFLAGS) core/chrono_power_shield.c core/common/chrono_shell_guard.c -o $@

build/chrono_panic_protocol: core/chrono_panic_protocol.c core/common/chrono_shell_guard.c core/common/chrono_shell_guard.h
	$(CC) $(CFLAGS) core/chrono_panic_protocol.c core/common/chrono_shell_guard.c -o $@

build/chrono_pin_forge: core/chrono_pin_forge.c
	$(CC) $(CFLAGS) core/chrono_pin_forge.c -o $@ $(LDFLAGS)

build/chrono_master: core/chrono_master_orchestrator.c core/common/chrono_shell_guard.c core/common/chrono_shell_guard.h
	$(CC) $(CFLAGS) core/chrono_master_orchestrator.c core/common/chrono_shell_guard.c -o $@

build/chrono_synthetic_life: core/chrono_synthetic_life.c
	$(CC) $(CFLAGS) core/chrono_synthetic_life.c -o $@ $(LDFLAGS)

check:
	@echo "===== CHRONOOS BUILD CHECK ====="
	@test -f VERSION
	@test -f Makefile
	@test -d core
	@test -d sentinel
	@test -x sentinel/sovereign/engine/chrono-threat-engine.sh
	@test -x sentinel/sovereign/engine/chrono-behavior-correlator.sh
	@test -x sentinel/sovereign/engine/chrono-incident-timeline.sh
	@test -d sentinel/sovereign/threats
	@test -d sentinel/sovereign/response
	@test -d sentinel/sovereign/tests
	@echo "VERSION=$$(cat VERSION)"
	@echo "Required structure: OK"
	@echo "Sentinel security structure: OK"
	@echo "CHECK PASSED"

test: build
	@echo "===== CHRONOOS TEST ====="
	@set -e; \
	for binary in $(BINARIES); do \
		test -x "$$binary"; \
		file "$$binary"; \
	done
	@echo "All required binaries built successfully"
	@echo "TEST PASSED"

clean:
	rm -rf build/*
	rm -rf var/*
	rm -rf vault/*
