CC = gcc
CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -lcrypto

all: directories build/chrono_power_shield build/chrono_panic_protocol build/chrono_pin_forge

directories:
	mkdir -p build core scripts vault/carrington_safe crypto_keys config var/run

build/chrono_power_shield: core/chrono_power_shield.c
	$(CC) $(CFLAGS) core/chrono_power_shield.c -o build/chrono_power_shield

build/chrono_panic_protocol: core/chrono_panic_protocol.c
	$(CC) $(CFLAGS) core/chrono_panic_protocol.c -o build/chrono_panic_protocol

build/chrono_pin_forge: core/chrono_pin_forge.c
	$(CC) $(CFLAGS) core/chrono_pin_forge.c -o build/chrono_pin_forge $(LDFLAGS)

clean:
	rm -rf build/* var/* vault/*

.PHONY: all directories clean
