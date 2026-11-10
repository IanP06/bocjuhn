# bocjuhn 1.0 RC8 — Effects / PRISM

Desarrollo local, 10 de noviembre de 2026. Paquete B108, identidad FM-1_197100.
RC7 fue probado por el propietario. **RC8 aún requiere escucha y medición en el FM1 físico.**

## Controles

SEQ recorre DELAY 1/2, DELAY 2/2, REVERB 1/2, REVERB 2/2, MOD,
PLAY ALONG, CLOCK y ACID si corresponde. Los cuatro knobs corresponden a
las cuatro filas. No hace falta mantener un botón para acceder a la segunda página.

| Efecto | Página 1 — K1 / K2 / K3 / K4 | Página 2 — K1 / K2 / K3 / K4 |
|---|---|---|
| Delay | TIME / REPEATS / DAMP / MIX | TYPE / DIVISION / RHYTHM / WIDTH |
| Reverb | SIZE / DECAY / DAMP / MIX | TYPE / PREDELAY / DIFFUSION / WIDTH |
| Granos / PRISM | SIZE / DENSITY / SCATTER / FEEDBACK | PITCH / REVERSE / FREEZE / PRISM MIX |

SHIFT + K2 abre GRAINS. Dentro de esa pantalla, SEQ alterna las dos páginas.
SHIFT + K2 sale; también se puede volver mediante una pista. PRISM MIX=0
conserva el granulator anterior; un valor positivo activa el modo nuevo.
No se modificaron los controles de FILTER, CRUSH ni DRIVE.

## Reverbs

Las seis variantes comparten el tanque de difusión existente para respetar
el presupuesto de memoria del FM1. Son variantes DSP propias, no emulaciones
de seis equipos diferentes:

- PLATE: la placa original, con modulación de las líneas.
- DIGITAL: tanque sin oscilación de las líneas, de carácter más fijo.
- SHIMMER: dos ventanas solapadas elevan una octava la excitación y parte del retorno.
- GRAIN CLOUD: ventanas de lectura inversa más rápidas antes de la difusión.
- REVERSE: dos ventanas inversas solapadas alimentan el tanque; no predice audio futuro.
- FROZEN HALL: realimentación muy larga y entrada atenuada; es una cola sostenida,
  no una captura infinita ni un freeze bit a bit.

PREDELAY 0–100 corresponde aproximadamente a 0–20 ms. DIFFUSION controla los
cuatro allpass de entrada. WIDTH 0 suma la salida a mono; 100 conserva la anchura.
MIX controla el retorno de un efecto por envío, no reemplaza la señal seca de las pistas.

## Delays

TAPE mantiene el deslizamiento original de tiempo. DIGITAL acelera ese ajuste.
PING PONG alterna la percepción de las repeticiones entre dos taps estéreo;
REVERSE usa dos ventanas inversas con crossfade. MULTITAP suma lecturas a
distintas distancias. Se comparte una línea mono de feedback; no son dos
líneas estéreo independientes.

DIVISION: FREE, 1/2, 1/4, 1/8, 1/16, 1/32. En FREE manda TIME; con una
división manda el BPM global. RHYTHM permite STRAIGHT, DOTTED (×1,5),
TRIPLET (×2/3) y HYBRID: tap izquierdo dotted y tap derecho triplet de la
misma división. La memoria limita el tiempo a dos segundos, incluso si una
división a BPM bajo pediría más. WIDTH controla la separación de los modos
con taps diferentes; los modos mono no generan anchura artificial.

## PRISM

Inspirado en la separación de tamaño, densidad, dispersión, dirección y
captura de [Arturia Efx FRAGMENTS](https://www.arturia.com/products/software-effects/efx-fragments/overview).
Es un desarrollo propio reducido a los recursos del FM1.

Con PRISM MIX > 0, SIZE abarca aproximadamente 4–400 ms, DENSITY controla el
intervalo entre nacimientos (250–2 ms), SCATTER la dispersión en el historial,
y FEEDBACK ajusta la recirculación hasta un máximo de 75 %. Los cuatro primeros
controles conservan su escala bipolar −100…100. El pitch va de −24 a +24 semitonos;
REVERSE es una probabilidad de 0–100 %, FREEZE detiene la escritura del historial
y MIX controla la mezcla. Se conservan hasta seis granos simultáneos por pista.

El historial es de unos 557 ms, mono y comprimido en mu-law. Cuando SIZE,
pitch y reverse exigirían más historial, la ventana se acorta para evitar
rechazar todos los granos. A densidades altas actúa el límite de seis voces.
Freeze puede sostenerse después de soltar las notas mientras PRISM siga activo.

## Modulación

La matriz pasa de 14 a 33 destinos. Se agregan:

- RV SIZE, DECAY, DAMP, MIX, PRE, DIFF y WIDTH.
- DL TIME, FEEDBACK, DAMP, MIX y WIDTH.
- GR SIZE, DENSITY, SCATTER, FEEDBACK, PITCH, REVERSE y MIX.

Los destinos de reverb/delay son globales: las rutas de las seis pistas se
suman y se limitan al rango permitido. Los destinos GR pertenecen a su pista.
Los valores modulados no sobrescriben los valores guardados. DL TIME actúa en
FREE; una división sincronizada determina el tiempo mediante el BPM.
Tipo, división y freeze se editan directamente; no se añadieron como destinos
continuos. Se mantienen cuatro slots y dos LFO por pista.

## Loops y cuantización

La referencia funcional distingue cuantizar el inicio de cuantizar la duración:
[FL Studio Quantizer](https://ww2.image-line.com/fl-studio-learning/fl-studio-online-manual/html/pianoroll_qnt.htm)
y [Ableton: Editing MIDI](https://www.ableton.com/en/manual/editing-midi/).

SYNC desplaza el inicio; el apagado viaja con su nota y conserva su gate en
el tiempo de la frase. Se corrigió la cancelación de notas anteriores de la
misma altura cuando se solapan: las voces polifónicas tienen ahora un apagado
asociado a su generación. ACID y los motores de comportamiento monofónico
conservan sus límites. La admisión de voces sigue dependiendo del presupuesto DSP.

Se agregan 1/8D, 1/16D, 1/32T y MIX 16+T. Esta última elige el punto más cercano
en la unión de semicorcheas rectas y semicorcheas de tresillo, permitiendo ambas
articulaciones en una frase. OFF conserva posiciones sin snap. Las notas del
final pueden caer en el siguiente primer tiempo sin recortarlas a ciclo−1.
Cambiar solamente la subdivisión ya no cancela los gates en curso.

SYNC sigue ajustando la frase a los compases de FIT: eso escala posiciones y
duraciones conjuntamente, como antes. Cambiar FIT o BPM es distinto de cambiar
la grilla. FREE recupera los eventos originales. Una nota sostenida al cerrar
REC sigue teniendo como final grabado el cierre de la toma; no se conoce su
duración futura. No se agregó cuantización de duración ni borrado de eventos.

## Guardado y comprobaciones

Sesiones 32772: extensión al final del formato 32771, con ajustes FX y sus
snapshots. Las sesiones RC7 cargan con placa y delay originales, PRISM apagado.
Los parámetros nuevos de escena se recuperan de forma discreta; los cuatro
parámetros antiguos conservan su morph. La copia de pista incluye PRISM.
Exportar un backup previo para regresar a RC7; no se promete lectura hacia atrás.

Pruebas: gates solapados, duraciones variadas, final de ciclo, grilla mixta,
navegación, suma de modulación, sesiones y snapshots, migración RC7, impulsos
en seis reverbs y veinte combinaciones de delay/ritmo, extremos de pitch y
freeze granular. También suite general de loops, macros, presets, escenas,
BALLSY, BRICKLAYER, sesiones, modulación, backup, y prueba WebAssembly.

Compilar con `tests/fx_clock.sh`; empaquetar con `tools/build_fx_release.py`.
Los resultados de host y navegador no miden la carga ni la latencia del FM1 real.
