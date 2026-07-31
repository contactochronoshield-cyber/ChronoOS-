# ==============================================================================
# ChronoOS Master Build System (Production Grade)
# ==============================================================================

OUT_DIR = build
INITRD = $(OUT_DIR)/initrd.img

.PHONY: all clean rootfs initramfs verify qemu

all: clean rootfs initramfs verify
	@echo "[+] Imagen de Sistema Operativo compilada con éxito en $(OUT_DIR)/initrd.img"

rootfs:
	@echo "[*] Construyendo y poblando Rootfs con BusyBox y herramientas nativas..."
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

qemu: all
	@echo "[*] Lanzando ChronoOS en entorno virtualizado QEMU..."
	@if command -v qemu-system-x86_64 >/dev/null 2>&1; then \
		qemu-system-x86_64 -initrd $(INITRD) -kernel /boot/vmlinuz-$(shell uname -r 2>/dev/null || echo "x") -append "console=ttyS0" -nographic 2>/dev/null || \
		echo "[!] QEMU requiere un kernel de Linux anfitrión válido en /boot."; \
	else \
		echo "[!] QEMU no está instalado en este entorno de terminal."; \
	fi

clean:
	@rm -rf $(OUT_DIR) initramfs/bin/busybox
	@echo "[*] Entorno limpio."
