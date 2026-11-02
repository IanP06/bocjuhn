# TONITOR / INTERVAL — decisiones y estado

Base: TONITOR, commit `0dccfcaed372368dd1519db22f4f4c5a109132d0`.

## Conservar

La interfaz permanece en inglés. Se retiró por completo la capa de traducción a español.

- BALLSY y BRICKLAYER originales, sin modificaciones ni otro generador euclidiano.
- PLAY ALONG: REACT, SHADOW y ARPEGGIO, con sus controles originales.
- Selección de pista, motores, familias, presets, generación de sonidos, ADSR y EDIT.
- Las cuatro macros específicas de cada motor/familia; siguen perteneciendo al sonido.
- Mod matrix, efectos existentes y grabación libre de loops superpuestos.

## Extensión implementada en el código

Velocidad continua independiente para cada pista y ajuste relativo para cada loop grabado.
La velocidad recorre los eventos de notas: no cambia la afinación del motor.

Desde la pantalla principal, con al menos un loop, cualquiera de las antiguas teclas
HALF/DOUBLE abre o cierra LOOP SPEED. En las páginas originales, como BRICKLAYER,
sus controles conservan su función. Seleccionar una pista vuelve a la pantalla habitual.

| Control en LOOP SPEED | Función |
| --- | --- |
| KNOB 1 | Velocidad de la pista: 25–400 % |
| KNOB 2 | Seleccionar uno de sus loops |
| KNOB 3 | Velocidad relativa del loop: 25–400 % |
| KNOB 4 | Sin asignación en esta página |

Las velocidades se multiplican: pista 150 % y loop 80 % dan 120 % efectivo.
La grabación sigue siendo libre; no se agrega grilla, cuantización ni tempo global.
Las sesiones, copiar/pegar y snapshots conservan ambos ajustes. Durante un morph,
las velocidades se recuperan junto con los loops en el punto medio.

El formato privado es 32769 y la cabecera INT2. Las sesiones y presets de TONITOR
original o de INTERVAL 0.1 no se importan: las dos ranuras de motor ahora representan
otras síntesis. Este cambio evita interpretar sonidos antiguos como motores nuevos.
El usuario confirmó que no necesita conservar datos del dispositivo.

## Granulación

Desde los controles de efectos habituales, SHIFT + KNOB 2 abre GRAINS. Los cuatro
knobs ajustan SIZE, DENSITY, SCATTER y FEEDBACK. Son desplazamientos de −100 a 100
respecto del carácter original del efecto; cero conserva ese parámetro original.
SIZE y DENSITY recorren aproximadamente un cuarto a cuatro veces el valor base,
con los límites del motor. SCATTER desplaza la dispersión y FEEDBACK el retorno,
limitado al 75 %. El knob GRAINS habitual sigue controlando el efecto global.

Seleccionar una pista, o repetir SHIFT + KNOB 2 desde esta página, vuelve a los controles
habituales. Los ajustes se guardan por pista en sesiones, escenas y copiar/pegar.

## Diseño de pantalla

Negro, blanco y acentos RGB/CMY. Liberation Sans reemplaza la tipografía de píxel:
números grandes en negrita, etiquetas regulares más discretas. Se incluyen ambos
archivos de fuente y su licencia. Las capturas se generan con el código C del firmware
a 240 × 240 y se amplían sin interpolación para la revisión. La función de ajuste
reduce el tamaño de los títulos largos antes de comprimir el espaciado.

Revisión: `build/screens/review-en.png`. Se retiró la propuesta de nombres españoles.

## Motores reemplazados — 1.0 RC1

PULSE ocupa la ranura interna de BAYAREA. Reproduce 16 muestras PCM originales,
sintetizadas de forma determinista por tools/gen_pulse.py, a 22050 Hz y 8 bits, con
interpolación a 44100 Hz. Las seis familias transforman este material en kits DRY,
METAL, DUST, RUBBER, LOGIC y VOID. Son 16 fuentes compartidas, no 96 grabaciones.
Macros: TONE, DECAY, GRIT, PUNCH. Cada nota dispara un one-shot; soltar la tecla no
corta la cola. El hat cerrado apaga suavemente el abierto de esa misma pista.
ADSR personalizado y efectos de TONITOR siguen disponibles.

Las 16 teclas blancas y MIDI 48–63: KICK, SUB, SNARE, CLAP, HAT, OPEN HAT, RIM, TOM,
CLICK, ZAP, METAL, SHARD, NOISE, BLOP, TICK, CRUNCH. Fuera de ese rango, MIDI se repite
módulo 16. Escala, raíz, octava, transposición de loops y acordes no cambian el pad.
BRICKLAYER conserva posiciones y tiempos; HARMONY puede desplazar la selección de
instrumento. BALLSY conserva su física y sus paredes eligen cuatro grupos de pads.
PLAY ALONG conserva sus modos, encaminando las notas generadas a los pads sin snap tonal.

FRACT ocupa la ranura interna de RUSSOLO: BLIPS, PACKET, LOGIC, BUFFER, GLASS, STATIC.
BLIPS conserva generación, osciladores, azar, envolvente y medida de nivel de TONITOR.
Las demás familias son síntesis nueva: portadoras en ráfagas, ondas digitales periódicas,
ciclos modulados, parciales inarmónicos e impulsos. Macros COLOR, GRAINS, RATE, SCATTER.
GRAINS es mezcla granular: centro seco, derecha hacia adelante, izquierda invertida.
RATE también ajusta el tamaño de grano; SCATTER cambia la posición de lectura y la
irregularidad del generador. Se reutiliza el búfer de 4096 bytes de la ranura original,
con dos lectores solapados, a 14700 Hz. El efecto granular normal de TONITOR sigue aparte.

La síntesis está en bayarea.c / pulse_body.h y russolo.c / fract_body.h. Se conservan
los nombres internos de las ranuras para limitar cambios en el mezclador y la mod matrix.
Cada motor tiene 24 defaults en la lista PRESETS original, sin página ni banco nuevo.
Los presets guardados reemplazan esos defaults; los espacios 25–64 quedan libres.
Los presets individuales conservan seed/familia/macros; las sesiones conservan además
ADSR, efectos, loops y escenas. T4 arranca con FRACT y T6 con PULSE. HIVE sigue disponible
mediante PRESETS; los otros seis motores no se reemplazan.

## Validación y límites

`tests/engines.sh` comprueba 96 combinaciones de kit/pad, identidad individual de los
pads, orden relativo de energía espectral entre kick/sub, snare y hat, colas one-shot,
choke, macros, granos normales/invertidos, señal acotada, determinismo, teclado,
escalas/octavas, adaptadores de BALLSY/BRICKLAYER/loops, 48 presets y recuperación de
sesión. Además compara 36 renders de BLIPS contra el commit original: coinciden bit a bit
con las macros al centro. No implica que una macro granular sea idéntica a RUSSOLO.

`tests/release_checks.sh` comprueba operaciones admitidas por el chip, velocidad,
controles granulares, flujos, latch, PLAY ALONG, loops, transposición, setup, backup,
presets, macros, overdubs, escenas, BALLSY, BRICKLAYER, 64 sesiones, render ANIMA golden
y el protocolo de instalación con un dispositivo simulado. Los sellos de pruebas
vinculan los resultados a las fuentes exactas. No se usa la prueba de importación de
sesiones originales, deliberadamente incompatibles con INT2; se prueba su rechazo.
La suite original tiene un fallo preexistente en paths, reproducido en el commit base.
Las pruebas históricas específicas de los antiguos BAYAREA/RUSSOLO tampoco describen
los motores nuevos. No se afirma que toda la suite original sea aplicable o haya pasado.

El navegador ejecuta este mismo DSP/UI C compilado con Emscripten. No simula tiempos del
AC791N ni carga del bus de flash. Hay que validar carga máxima, autonomía, arranque,
guardado/reinicio y recuperación en el equipo real. La escucha de los ejemplos debe
guiar el ajuste musical: las mediciones de señal no sustituyen esa evaluación.

## Construcción e instalación

Base: TONITOR 0dccfcaed372368dd1519db22f4f4c5a109132d0, build 68. Toolchain JieLi
20260730.1; SDK extraído del paquete original con comprobación de SHA-256. Identidad
privada FM-1_197100, versión I10r, archivo interval-1.0-rc1.fwsc. No está publicado,
registrado ni certificado en Firmware Hub. La guía está en INSTALL-ES.txt.

Comandos dentro del contenedor, directorio /work:

```
sh tests/interval-build.sh
sh tests/engines.sh > build/engines/results.txt
python3 tools/build.py --interval
sh tests/release_checks.sh > build/release-checks.log
sh tests/build_browser.sh
python3 tools/package_interval.py
```

El toolchain y SDK se indican mediante JIELI_TOOLCHAIN y AC79_SDK. Los Dockerfiles y
prepare_target.sh están incluidos. El paquete se niega a cerrar con fuentes o pruebas
obsoletas. manifest.json contiene las cifras exactas de enlace, hashes y procedencia.
No se modifican los 33 archivos críticos de HAL, USB, arranque, loader ni protocolo.
Se mantiene la caché ANIMA de 12 tablas de la extensión anterior, con render golden
idéntico; no se reducen voces, loops o escenas. El margen de memoria es ajustado.
POOL incluye 8192 bytes reservados para la pila de CPU1, además de su alineación.

El instalador local comprueba estructura, tamaño, identidad y SHA-256. Leer identidad
es independiente de instalar. La escritura usa el protocolo original y requiere la
acción explícita del usuario. No se ha accedido al hardware durante esta preparación.


## RC2 — RYUJI (31-10-2026)

El usuario confirmó arranque y funcionamiento de RC1 en su FM1. Detectó que PULSE no respondía correctamente al ADSR ni a las octavas y que sus macros tenían poco efecto audible.

RYUJI conserva la ranura BAYAREA, el PCM original y el formato INT2/32769. El preset 1 es KANAGAWA. El registro de pista se publica junto con las macros y se aplica a cada voz, también durante una nota sostenida. BALLSY codifica su octava en bancos de 16 notas conservando el pad; MIDI 48–63 sigue siendo el banco central. Los loops conservan ese banco.

Con ADSR personalizado, dos granos triangulares solapados leen una región temprana del cuerpo. La transición inicial tarda unos 23 ms. El cuerpo no se agota al final del PCM: la envolvente determina la vida de la voz. Sin ADSR personalizado se conserva la reproducción one-shot. Activar ADSR durante una voz todavía viva habilita la continuación; una voz terminada requiere un nuevo disparo.

STRUCTURE mezcla plegado simétrico con la señal original. DAMPING cierra el segundo filtro progresivamente después del ataque; BRIGHTNESS establece el límite espectral del primero. POSITION mezcla una lectura desplazada con polaridad variable. Un filtro elimina DC y la salida sigue acotada. No se añaden búferes de audio por voz.

Las pruebas RC2 comprueban diferencias de forma de onda normalizadas por ganancia para las cuatro macros en 16 pads, etapas ADSR sobre el mezclador real, colas audibles más de un segundo después de soltar, fin de release, OCT sobre notas sostenidas, registro BALLSY y nombres. El coste y la escucha de RC2 en el FM1 siguen pendientes.

Los valores de las macros guardados en RC1 tienen ahora otra interpretación sonora. Paquete RC2: interval-1.0-rc2.fwsc, versión I102, identidad FM-1_197100. RC1 permanece en su carpeta de release.
