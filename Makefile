CC = gcc
CFLAGS = -Wall -Wextra -O2

all: clean build

build:
	@echo "[*] Compilando componentes del sistema operativo en C..."
	mkdir -p bin build
	$(CC) $(CFLAGS) core/engine/chrono_core.c -o bin/chrono-core
	$(CC) $(CFLAGS) init/chrono_init.c -o build/chrono_init
	@echo "[✓] Compilación nativa completada con éxito."

clean:
	@echo "[*] Limpiando artefactos temporales..."
	rm -rf build/* bin/*
