#!/bin/sh
echo "[*] Lanzando proceso bajo entorno aislado (Sandbox ChronoOS)..."
unshare --pid --net --mount-proc --fork --propagation private /bin/sh
