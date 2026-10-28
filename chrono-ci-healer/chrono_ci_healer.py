#!/usr/bin/env python3
"""Chrono CI Healer: diagnóstico CI y reparaciones seguras y limitadas."""

import argparse
import re
import subprocess
import sys
from pathlib import Path

RULES = [
    (
        "PYTHON_BYTECODE",
        r"Python bytecode present|\.pyc\b|\.pyo\b",
        "Se detectó bytecode Python.",
        "Revisa los archivos .pyc/.pyo no versionados y las reglas de .gitignore.",
    ),
    (
        "YAML_WORKFLOW",
        r"Invalid workflow file|yaml\.parser|YAMLException|"
        r"Unexpected value|No jobs found|jobs is required",
        "Posible error de sintaxis o estructura en GitHub Actions.",
        "Valida la indentación YAML, los eventos, permisos y la sección jobs.",
    ),
    (
        "DEPENDENCY",
        r"ModuleNotFoundError|No module named|Could not resolve|"
        r"dependency.*not found",
        "Posible dependencia ausente.",
        "Comprueba el archivo de dependencias y la versión del entorno.",
    ),
    (
        "GRADLE",
        r"Gradle build failed|Could not resolve all files|"
        r"Execution failed for task|BUILD FAILED",
        "Falló una tarea o dependencia de Gradle.",
        "Inspecciona la primera causa del error y vuelve a ejecutar con --stacktrace.",
    ),
    (
        "COMPILATION",
        r"fatal error:|undefined reference|error: .* undeclared|"
        r"Compilation failed",
        "Posible error de compilación.",
        "Revisa el primer error del compilador antes de corregir errores derivados.",
    ),
    (
        "TEST_FAILURE",
        r"FAILED|AssertionError|tests failed|"
        r"failed \(.*tests?\)",
        "Una o más pruebas fallaron.",
        "Identifica la prueba y reproduce el fallo antes de cambiar el código.",
    ),
    (
        "PERMISSION",
        r"Permission denied|EACCES|not permitted",
        "Posible problema de permisos.",
        "Comprueba propietario, permisos y rutas; evita aplicar chmod indiscriminadamente.",
    ),
    (
        "NETWORK",
        r"Connection timed out|Temporary failure in name resolution|"
        r"Connection reset by peer",
        "Posible fallo temporal de red.",
        "Comprueba conectividad y disponibilidad del servicio antes de modificar el proyecto.",
    ),
]

def diagnose(log_path):
    path = Path(log_path)
    if not path.is_file():
        print("ERROR: no existe el archivo de log:", path)
        return 2

    content = path.read_text(encoding="utf-8", errors="replace")
    found = []
    for code, pattern, title, advice in RULES:
        matches = list(re.finditer(pattern, content, re.IGNORECASE))
        if matches:
            line = content.count("\n", 0, matches[0].start()) + 1
            found.append((code, title, advice, line))

    print("=== CHRONO CI HEALER / DIAGNOSTICO ===")
    print("Log:", path)
    if not found:
        print("No se reconocieron patrones conocidos.")
        print("No se aplicaron cambios. Requiere inspección del log.")
        return 0

    for code, title, advice, line in found:
        print(f"\n[{code}] Línea aproximada: {line}")
        print(title)
        print("Siguiente paso:", advice)

    print("\nDiagnóstico orientativo: confirma la causa con el log completo.")
    print("No se modificó ningún archivo.")
    return 0

def tracked_files(root):
    try:
        result = subprocess.run(
            ["git", "-C", str(root), "ls-files", "-z"],
            capture_output=True, check=True
        )
        return {
            item.decode("utf-8", errors="surrogateescape")
            for item in result.stdout.split(b"\0") if item
        }
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None

def fix_python_cache(root, apply=False):
    root = Path(root).resolve()
    tracked = tracked_files(root)
    if tracked is None:
        print("ERROR: no se pudo consultar Git. No se modificó nada.")
        return 2

    candidates = []
    for pattern in ("*.pyc", "*.pyo"):
        for path in root.rglob(pattern):
            if ".git" in path.parts:
                continue
            relative = path.relative_to(root).as_posix()
            if relative not in tracked:
                candidates.append(path)

    # Eliminar duplicados sin cambiar el orden.
    candidates = list(dict.fromkeys(candidates))
    print(f"Bytecode no versionado encontrado: {len(candidates)}")
    for path in candidates:
        print(("Eliminar: " if apply else "Candidato: "), path.relative_to(root))
        if apply:
            path.unlink(missing_ok=True)

    if apply:
        for path in sorted(root.rglob("__pycache__"), reverse=True):
            if ".git" in path.parts or not path.is_dir():
                continue
            try:
                path.rmdir()  # Solo elimina carpetas que quedaron vacías.
            except OSError:
                pass
        print("Limpieza limitada terminada.")
    else:
        print("Modo de revisión: no se borró nada.")
        print("Para aplicar: fix-safe --apply RUTA_DEL_REPOSITORIO")

    return 0

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    diag = sub.add_parser("diagnose", help="Analizar un log de CI")
    diag.add_argument("log")

    fix = sub.add_parser("fix-safe", help="Limpiar bytecode no versionado")
    fix.add_argument("root", nargs="?", default=".")
    fix.add_argument("--apply", action="store_true",
                     help="Aplicar la limpieza; por defecto solo inspecciona")

    args = parser.parse_args()
    if args.command == "diagnose":
        return diagnose(args.log)
    return fix_python_cache(args.root, args.apply)

if __name__ == "__main__":
    sys.exit(main())
