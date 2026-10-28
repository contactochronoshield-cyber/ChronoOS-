# ChronoOS
## Infraestructura de ciberseguridad, telecomunicaciones y computación soberana

ChronoOS es un proyecto del ecosistema Chrono Shield que reúne componentes
de software para seguridad, registro de eventos, conectividad, IoT,
infraestructura de borde y herramientas de desarrollo.

**Organización:** Chrono Shield Systems  
**Infraestructura y telecomunicaciones:** Chrono Shield Networks  
**Licencias y autorizaciones:** contactochronoshield@gmail.com

> Los componentes tienen diferentes niveles de madurez. Que exista código
> o una demo no significa que esté auditado ni listo para producción.

## 1. Componentes del proyecto

- **Núcleo y herramientas del sistema:** componentes de infraestructura y
  diagnóstico.
- **Chrono Ledger:** registro de eventos con encadenamiento de hashes para
  ayudar a detectar alteraciones.
- **Sentinel:** herramientas de supervisión, análisis de eventos e indicadores
  de seguridad; las capacidades dependen de cada módulo.
- **Chrono IoT Engine:** procesamiento de eventos relacionados con dispositivos.
- **Chrono CAN Guard:** análisis y demostración de escenarios CAN simulados.
- **ChronoOS 1.3:** CLI, cola persistente de eventos, receptor de lotes firmados,
  identidad de nodos y verificación de actualizaciones.
- **Chrono CI Healer:** diagnóstico de ciertos errores de integración continua
  mediante registros y una acción reutilizable de GitHub.
- **Chrononet y edge:** carpetas relacionadas con conectividad, dispositivos,
  sensores y computación de borde.

Consulta la documentación de cada componente para conocer su estado real,
requisitos y limitaciones.

## 2. Compilación y pruebas

Desde la raíz del repositorio:

```bash
make build
make check
make test
```

Estos comandos ejecutan únicamente los objetivos y pruebas definidos en el
Makefile; no certifican todos los componentes ni todas las plataformas.

Para comprobar el ledger, si está compilado:

```bash
./bin/chrono-ledger verify
```

Conserva los registros y revisa cualquier error. Una verificación correcta
no equivale a una auditoría de seguridad completa.

## 3. Demo: Chrono CAN Guard

Ejecuta desde la raíz del repositorio:

```bash
make build
./bin/chrono-can-guard demo
```

La demostración presenta tráfico y anomalías simuladas, y puede registrar
eventos en el ledger según la implementación actual.

**Importante:** la demo no transmite tramas a un vehículo real. No demuestra
compatibilidad universal ni certifica seguridad automotriz.

## 4. Demo: Chrono IoT Engine

Ejecuta desde la raíz:

```bash
make build
./bin/chrono-iot-demo
```

El script está destinado a mostrar un escenario de demostración del motor IoT.
Revisa su salida y el código del script para confirmar exactamente qué datos
procesa en la versión instalada.

Los datos simulados no prueban que existan sensores físicos conectados ni
validan una instalación real.

## 5. Evaluación en un laboratorio o en el hogar

La evaluación doméstica o en un laboratorio interno solo está permitida cuando
Chrono Shield Networks haya otorgado previamente autorización escrita.

Para una evaluación autorizada:

1. Identifica el módulo y revisa su documentación.
2. Usa datos ficticios y un entorno aislado.
3. Ejecuta las demos y pruebas disponibles.
4. Conserva los registros y documenta los errores.
5. No conectes sistemas críticos sin una evaluación específica de seguridad.
6. No expongas servicios de red a redes no confiables.

El acceso público al repositorio no concede autorización para utilizarlo.

## 6. Producción y uso institucional

Todo despliegue empresarial, institucional o de producción requiere autorización
de Chrono Shield Networks y los acuerdos contractuales aplicables.

Antes de un despliegue se debe evaluar:

- Auditoría de seguridad y dependencias.
- Control de acceso y modelo de amenazas.
- Cifrado del transporte de red.
- Protección, almacenamiento y rotación de claves.
- Copias de seguridad, recuperación y retención de registros.
- Pruebas de carga, fallos y actualizaciones.
- Compatibilidad con el hardware y el sistema operativo de destino.
- Requisitos regulatorios y contractuales.

Las firmas digitales ayudan a comprobar autenticidad e integridad, pero no
cifran por sí solas el tráfico. No se debe afirmar que el proyecto está
certificado o listo para producción sin evidencias que lo acrediten.

## 7. ChronoOS 1.3 en Termux

Esta guía corresponde al módulo `chronoos-1.3`; no instala un sistema operativo
completo ni otorga control ilimitado sobre Android.

Instala los requisitos documentados:

```bash
pkg update
pkg install python python-cryptography
```

Desde la raíz del repositorio:

```bash
chmod +x chronoos-1.3/chrono
mkdir -p ~/bin
ln -sf "$PWD/chronoos-1.3/chrono" ~/bin/chrono
```

Si `~/bin` no está en PATH:

```bash
echo 'export PATH="$HOME/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Comprueba la instalación:

```bash
chrono --help
chrono doctor
chrono status
chrono caps
```

Comandos documentados para explorar sus funciones:

```bash
chrono queue
chrono logs
chrono event KIND "mensaje"
chrono daemon start
chrono daemon status
chrono daemon stop
chrono node
chrono server run
chrono server list
```

Consulta `chronoos-1.3/README.md` antes de ejecutar operaciones de servidor,
enrolamiento, revocación o actualización. Protege las claves privadas y usa
transporte cifrado y controles de acceso apropiados.

## 8. Pruebas de ChronoOS 1.3

```bash
python3 chronoos-1.3/tests_cli.py
python3 chronoos-1.3/tests_phase4.py
```

Una prueba satisfactoria cubre solamente los casos que se ejecutan.

## 9. Documentación adicional

- `chronoos-1.3/README.md`
- `chronoos-1.3/SECURITY-AND-READINESS.md`
- `platforms/termux/README.md`
- `sentinel/README.md`
- `chrono-ci-healer/README.md`
- `IMPLEMENTATION_STATUS.md`, si está presente

Termux es un entorno de ejecución dentro de Android, no una validación
del sistema completo en hardware de producción.

## 10. Licencia propietaria y autorización obligatoria

ChronoOS está sujeto a los términos de la licencia propietaria del archivo
`LICENSE`. El código, la documentación y los activos cubiertos por ella son
propiedad de Chrono Shield Networks, conforme a dicha licencia.

**La publicación del repositorio en GitHub no concede una licencia general
de uso.** Según los términos de `LICENSE`, el uso personal o comercial,
la copia, modificación, creación de derivados, distribución e ingeniería
inversa requieren autorización previa y escrita.

Por tanto, las pruebas personales, las demos y el uso interno en el hogar
también requieren autorización escrita previa. Los despliegues de producción
requieren además los acuerdos contractuales aplicables.

Solicita autorización en:

**contactochronoshield@gmail.com**

No interpretes la disponibilidad pública del código como una licencia de
software libre ni como permiso automático para reutilizarlo.

## 11. Estado del proyecto

ChronoOS continúa en desarrollo. Los componentes deben evaluarse de forma
individual según su implementación, pruebas, documentación, compatibilidad
y madurez. Las demos no sustituyen una auditoría ni una autorización de uso.
