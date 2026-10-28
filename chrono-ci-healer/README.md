# Chrono CI Healer

Motor inicial de diagnóstico de fallos de integración continua (CI).

## Requisitos
- Python 3.8 o posterior
- Git para comprobar qué archivos están versionados
- Sin dependencias Python externas

## Diagnosticar un log
    python3 chrono-ci-healer/chrono_ci_healer.py diagnose ruta/al/error.log

## Revisar bytecode Python
    python3 chrono-ci-healer/chrono_ci_healer.py fix-safe .

## Aplicar limpieza limitada
    python3 chrono-ci-healer/chrono_ci_healer.py fix-safe --apply .

La limpieza solo elimina archivos .pyc/.pyo no versionados y después
intenta retirar directorios __pycache__ vacíos. No repara código arbitrario.

## Arquitectura prevista
1. Detectar fallos de GitHub Actions.
2. Obtener logs del flujo fallido con permisos mínimos.
3. Clasificar la causa y adjuntar evidencia.
4. Aplicar únicamente reparaciones aprobadas para patrones conocidos.
5. Reejecutar pruebas y abrir una rama/PR si la reparación pasa.
6. Nunca forzar cambios directamente en main.

La detección por patrones es una primera versión, no una IA capaz de
resolver cualquier fallo. Las reparaciones futuras deben tener pruebas,
límites de permisos y revisión de cambios.

## Reusable GitHub Action

The composite action accepts a log file that an existing workflow has
already produced. Add this step to a workflow after a step that saves its
output to `ci.log`:

    - name: Diagnose failed CI
      if: failure()
      uses: ./chrono-ci-healer
      with:
        log_path: ci.log

This action diagnoses logs; it does not automatically repair source code
or make commits. The log must exist in the runner workspace. For another
repository, copy this directory into that repository or publish it as its
own repository and reference a pinned version.
