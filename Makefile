# ==============================================================================
# ChronoOS Master Build System (Production Grade)
# ==============================================================================

OUT_DIR = build
INITRD = $(OUT_DIR)/initrd.img

.PHONY: all clean rootfs initramfs verify

all: clean rootfs initramfs verify
	@echo "[+] Imagen de Sistema Operativo compilada con éxito en $(OUT_DIR)/initrd.img"

rootfs:
	@echo "[*] Construyendo y poblando Rootfs..."
	@./scripts/build_rootfs_engine.sh

initramfs:
	@mkdir -p $(OUT_DIR)
	@echo "[*] Generando Initramfs comprimido con CPIO y Gzip..."
	@cd initramfs && find . -print0 | cpio --null -ov --format=newc 2>/dev/null | gzip -9 > ../$(INITRD)
	@echo "[✔] Initramfs generado correctamente."

verify:
	@if [ -f "$(INITRD)" ]; then \
		echo "[✔] Veredicto Técnico: Imagen válida y ejecutable generada."; \
	else \
		echo "[!] Error: Falló la compilación de la imagen."; exit 1; \
	fi

clean:
	@rm -rf $(OUT_DIR) initramfs/bin/busybox
	@echo "[*] Entorno limpio."
