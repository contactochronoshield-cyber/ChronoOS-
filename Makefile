# ==============================================================================
# ChronoOS Master Makefile
# Author: Daniel Gonzales / Chrono Shield Networks
# ==============================================================================

.PHONY: all clean initrd test

all: initrd

initrd:
	@echo "[*] Ejecutando motor de empaquetado para ChronoOS..."
	@./scripts/pack_initrd.sh

test: initrd
	@if [ -f "./run-qemu.sh" ]; then
		./run-qemu.sh
	else
		echo "[!] Script run-qemu.sh no encontrado. Initrd listo en build/initrd.img"
	fi

clean:
	@echo "[*] Limpiando artefactos de compilación..."
	@rm -rf build/*.img build/*.cpio.gz
	@echo "[✓] Limpieza completada."
