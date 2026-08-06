CC = gcc
CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -lcrypto

all: directories build/chrono_power_shield build/chrono_panic_protocol build/chrono_pin_forge build/chrono_master build/chrono_synthetic_life

directories:
	mkdir -p build core scripts vault/carrington_safe vault/sentient_entities crypto_keys config var/run

build/chrono_power_shield: core/chrono_power_shield.c
	$(CC) $(CFLAGS) core/chrono_power_shield.c -o build/chrono_power_shield

build/chrono_panic_protocol: core/chrono_panic_protocol.c
	$(CC) $(CFLAGS) core/chrono_panic_protocol.c -o build/chrono_panic_protocol

build/chrono_pin_forge: core/chrono_pin_forge.c
	$(CC) $(CFLAGS) core/chrono_pin_forge.c -o build/chrono_pin_forge $(LDFLAGS)

build/chrono_master: core/chrono_master_orchestrator.c
	$(CC) $(CFLAGS) core/chrono_master_orchestrator.c -o build/chrono_master

build/chrono_synthetic_life: core/chrono_synthetic_life.c
	$(CC) $(CFLAGS) core/chrono_synthetic_life.c -o build/chrono_synthetic_life $(LDFLAGS)

clean:
	rm -rf build/* var/* vault/*

.PHONY: all directories clean
