# ==============================================================================
# ChronoOS Master Build System
# ==============================================================================

ARCH ?= x86_64
OUT_DIR = build
ISO_IMAGE = $(OUT_DIR)/chronoos-$(ARCH).iso

.PHONY: all clean prepare initramfs

all: clean prepare initramfs
	@echo "[+] Construcción de ChronoOS finalizada para $(ARCH)."

prepare:
	@mkdir -p $(OUT_DIR)
	@echo "[*] Preparando entorno de compilación cruzada..."

initramfs:
	@echo "[*] Empaquetando Initramfs nativo..."
	@cd initramfs && find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../$(OUT_DIR)/initrd.img
	@echo "[✔] Initramfs generado con éxito en $(OUT_DIR)/initrd.img"

clean:
	@echo "[*] Limpiando artefactos de compilación..."
	@rm -rf $(OUT_DIR)
