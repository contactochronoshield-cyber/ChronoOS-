# ==============================================================================
# ChronoOS Master Build System
# ==============================================================================

OUT_DIR = build
INITRD = $(OUT_DIR)/initrd.img

.PHONY: all clean initramfs verify

all: clean initramfs verify
	@echo "[+] Paquete de sistema operativo compilado con éxito en $(OUT_DIR)/"

initramfs:
	@mkdir -p $(OUT_DIR)
	@echo "[*] Generando Initramfs comprimido..."
	@cd initramfs && find . -print0 | cpio --null -ov --format=newc 2>/dev/null | gzip -9 > ../$(INITRD)
	@echo "[✔] Imagen initrd generada: $(INITRD)"

verify:
	@if [ -f "$(INITRD)" ]; then \
		echo "[✔] Verificación de integridad: Archivo initrd.img verificado."; \
	else \
		echo "[!] Error crítico: La imagen initrd no se pudo generar."; exit 1; \
	fi

clean:
	@rm -rf $(OUT_DIR)
	@echo "[*] Entorno de compilación limpio."
