# ChronoOS (Sovereign Infrastructure Toolkit & Operating System Base)

**ChronoOS** es un proyecto activo enfocado en la construcción de un entorno operativo soberano, minimalista y orientado a la ciberseguridad, gestión de bóvedas cifradas y redes descentralizadas.

## 🛠️ Especificaciones Técnicas del Repositorio
* **Subsistema de Arranque:** Init nativo autónomo en initramfs (PID 1).
* **Criptografía:** Rutinas integradas basadas en algoritmos estándar de la industria (`AES-256`, `SHA-256`).
* **Automatización:** Makefile maestro de compilación cruzada y empaquetado de imágenes de disco.
* **Entornos Soportados:** Servidores Bare-Metal x86_64 y entornos móviles aislados ARM64 (Termux).

## 🚀 Compilación Local
Para compilar la imagen de disco y el sistema de archivos raíz inicial:
```bash
make all



