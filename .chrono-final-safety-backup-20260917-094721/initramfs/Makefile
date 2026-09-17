CC = gcc
CFLAGS = -Wall -Wextra -static -O2

all: clean build_engine build_init initrd

build_engine:
	@echo "[*] Compilando motores C de bajo nivel..."
	mkdir -p bin initramfs/bin
	$(CC) $(CFLAGS) core/engine/chrono_core.c -o bin/chrono-core
	$(CC) $(CFLAGS) core/engine/chrono_packet_auditor.c -o bin/chrono-packet-auditor
	$(CC) $(CFLAGS) core/engine/chrono_volatile_shield.c -o bin/chrono-volatile-shield
	cp bin/chrono-core initramfs/bin/chrono-core
	cp bin/chrono-packet-auditor initramfs/bin/chrono-packet-auditor
	cp bin/chrono-volatile-shield initramfs/bin/chrono-volatile-shield

build_init:
	@echo "[*] Compilando proceso PID 1 (ChronoInit)..."
	mkdir -p build initramfs_root/sbin
	$(CC) $(CFLAGS) init/chrono_init.c -o build/chrono_init
	cp build/chrono_init initramfs_root/sbin/init
	chmod +x initramfs_root/sbin/init

initrd:
	@echo "[*] Generando imagen de disco inicial (initrd.img)..."
	if [ -d "initramfs" ]; then \
		cd initramfs && find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../build/initrd.img; \
	fi
	@echo "[✓] ChronoOS compilado y empaquetado con éxito."

clean:
	@echo "[*] Limpiando artefactos previos..."
	rm -rf build/* bin/*
