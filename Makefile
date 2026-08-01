CC = gcc
CFLAGS = -Wall -O2

all: chrono_init

chrono_init: init/chrono_init.c
	$(CC) $(CFLAGS) init/chrono_init.c -o build/chrono_init

install: all
	mkdir -p /etc/chrono /var/lib/chrono/pkg /usr/bin
	cp build/chrono_init /sbin/init
	cp bin/chrono-pkg /usr/bin/chrono-pkg
	chmod +x /usr/bin/chrono-pkg
	echo "[+] ChronoOS core desplegado y estructurado."

clean:
	rm -f build/chrono_init
