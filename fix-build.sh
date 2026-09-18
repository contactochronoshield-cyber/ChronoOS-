#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

echo "========================================"
echo " CHRONOOS BUILD REPAIR"
echo "========================================"

MAKEFILE="Makefile"
GUARD_C="core/common/chrono_shell_guard.c"
GUARD_H="core/common/chrono_shell_guard.h"

# --------------------------------------------------
# 1. Verify security guard
# --------------------------------------------------

echo
echo "[1/6] Verificando Shell Guard..."

if [[ ! -f "$GUARD_C" ]]; then
    echo "ERROR: falta $GUARD_C"
    exit 1
fi

if [[ ! -f "$GUARD_H" ]]; then
    echo "ERROR: falta $GUARD_H"
    exit 1
fi

if ! grep -q "int chrono_system_disabled" "$GUARD_C"; then
    echo "ERROR: chrono_system_disabled() no está definido correctamente."
    exit 1
fi

if ! grep -q "errno = EPERM" "$GUARD_C"; then
    echo "ERROR: el Shell Guard no conserva la denegación EPERM."
    exit 1
fi

echo "OK: Shell Guard presente y ejecución de comandos bloqueada."

# --------------------------------------------------
# 2. Detect source files using the guard
# --------------------------------------------------

echo
echo "[2/6] Detectando dependencias..."

mapfile -t USERS < <(
    grep -RIl \
        --exclude-dir=.git \
        --exclude-dir=build \
        --exclude-dir=build_root \
        "chrono_system_disabled(" \
        core \
        --include="*.c" 2>/dev/null | sort
)

if [[ ${#USERS[@]} -eq 0 ]]; then
    echo "No se encontraron consumidores de chrono_system_disabled()."
else
    echo "Archivos que utilizan chrono_system_disabled():"
    printf '  %s\n' "${USERS[@]}"
fi

# --------------------------------------------------
# 3. Backup Makefile
# --------------------------------------------------

echo
echo "[3/6] Creando respaldo del Makefile..."

cp "$MAKEFILE" "$MAKEFILE.backup.$(date +%Y%m%d%H%M%S)"

# --------------------------------------------------
# 4. Automatically add shell guard to existing
#    build rules when their source uses it.
# --------------------------------------------------

echo
echo "[4/6] Corrigiendo reglas de enlace..."

python3 <<'PY'
from pathlib import Path
import re

makefile = Path("Makefile")
text = makefile.read_text()

guard_c = "core/common/chrono_shell_guard.c"
guard_h = "core/common/chrono_shell_guard.h"

lines = text.splitlines()
out = []

i = 0

while i < len(lines):
    line = lines[i]

    # Detect target rules:
    # build/foo: source1 source2
    m = re.match(r"^(build/[A-Za-z0-9_.-]+):\s*(.*)$", line)

    if not m:
        out.append(line)
        i += 1
        continue

    target = m.group(1)
    deps = m.group(2).strip()

    # Capture recipe lines belonging to this target.
    block = [line]
    i += 1

    while i < len(lines):
        nxt = lines[i]

        if nxt.startswith("\t"):
            block.append(nxt)
            i += 1
            continue

        if re.match(r"^(build/[A-Za-z0-9_.-]+):", nxt):
            break

        if nxt.strip() == "":
            block.append(nxt)
            i += 1
            continue

        break

    # Only repair build rules whose source files actually
    # reference chrono_system_disabled().
    source_files = re.findall(r"(?:^|\s)(core/[A-Za-z0-9_./-]+\.c)(?:\s|$)", deps)

    needs_guard = False

    for src in source_files:
        p = Path(src)
        if p.exists() and "chrono_system_disabled(" in p.read_text():
            needs_guard = True
            break

    if needs_guard:
        if guard_c not in deps:
            deps = (deps + " " + guard_c + " " + guard_h).strip()

        block[0] = f"{target}: {deps}"

        # Add the guard C source to the compiler invocation
        # only when the recipe compiles the target directly.
        for n in range(1, len(block)):
            recipe = block[n]

            if recipe.startswith("\t") and "$(CC)" in recipe:
                if guard_c not in recipe:
                    recipe = recipe.replace(
                        " -o $@",
                        f" {guard_c} -o $@"
                    )
                    recipe = recipe.replace(
                        " -o build/",
                        f" {guard_c} -o build/"
                    )
                    block[n] = recipe

    out.extend(block)

text = "\n".join(out) + "\n"
makefile.write_text(text)
PY

# --------------------------------------------------
# 5. Show repaired rules
# --------------------------------------------------

echo
echo "[5/6] Reglas reparadas:"

grep -n -A2 \
    "build/chrono_power_shield:\|build/chrono_panic_protocol:\|build/chrono_master:" \
    Makefile || true

# --------------------------------------------------
# 6. Validate
# --------------------------------------------------

echo
echo "[6/6] Ejecutando validación completa..."
echo

make clean
make check
make test
make build

echo
echo "========================================"
echo " CHRONOOS BUILD REPAIR: COMPLETADO"
echo "========================================"

echo
echo "Estado Git:"
git status --short

echo
echo "Binarios generados:"
find build -maxdepth 1 -type f -perm -111 -print | sort
