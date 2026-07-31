SHELL := /bin/bash
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

.PHONY: all clean test install

all:
	@echo "ChronoOS v7.0.0 Titanium - Build System"
	@echo "Usa 'make install' para desplegar o 'make test' para verificar integridad."

test:
	@echo "[*] Ejecutando pruebas automatizadas básicas de scripts..."
	@bash tests/run_tests.sh

install:
	@echo "[*] Instalando ChronoOS en el sistema..."
	@install -Dm755 core/chrono $(BINDIR)/chrono
	@echo "[✔] Instalación completada con éxito."

clean:
	@echo "[*] Limpiando archivos temporales de compilación..."
	@rm -rf build_root/* vault_store/*.tmp 2>/dev/null || true
	@echo "[✔] Limpieza finalizada."
