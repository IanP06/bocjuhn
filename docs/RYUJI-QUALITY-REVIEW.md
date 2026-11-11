# RYUJI: revisión de calidad sonora (RC9)

Fecha: 2026-11-11. Base: e7f7201c2057de0e605ac76c3cb021de4890ddc7.

## Conclusión

Las fuentes actuales son aproximaciones sintéticas, no muestras originales ni modelos de circuito fieles. El problema principal está en la generación y el procesamiento por defecto; cambiar únicamente la resolución PCM no lo resuelve. Se recomienda un motor híbrido: VA específica para cuerpos de percusión, PCM de buena calidad para los instrumentos que lo necesitan, y reproducción neutra por defecto. Este estudio no cambia el firmware ni constituye una nueva release.

## Hallazgos del código

- `tools/gen_pulse.py` genera 48 fuentes con fórmulas simplificadas, cuantizadas a PCM firmado de 8 bits. Las familias 808/909 usan 22050 Hz. No contienen capturas de las máquinas.
- Los modelos metálicos mezclan osciladores sin reproducir la cadena de filtros y VCA del instrumento. Las discontinuidades de los cuadrados generan aliasing; el resultado tiene componentes tonales graves ajenas a las referencias examinadas.
- Algunas fuentes LM/727 reciben degradación adicional por cuantización y retención de muestras, sin evidencia de que reproduzca la máquina correspondiente.
- En `pulse_body.h`, macros en cero todavía introducen drive, aproximadamente 25% de señal plegada y filtrado. La envolvente nativa vuelve a acortar un PCM que ya tiene su propia caída.
- RC9 corrigió el bucle corto que añadía tono al sostener una fuente. Esto no arregla la calidad de la fuente ni permite prolongar una cola ya truncada.

## Comparación medida

Se exportó el renderer C real de RC9, sin efectos ni mezclador, y también la fuente sin procesar. Se compararon capturas etiquetadas TR-808 Clean y TR-909 Clean de [RAUM45](https://www.raum45.de/en/downloads/), PCM16/44.1 kHz. No son ROM oficiales de Roland. No se pudo verificar la unidad física, cadena de captura ni posiciones de controles. Los archivos múltiples no deben interpretarse como unidades o ajustes independientes. Las diferencias son evidencia útil para esta selección, no una prueba universal de autenticidad.

T95 es el tiempo que acumula el 95% de la energía desde el inicio; no la duración audible total. El centroide usa energía espectral Welch. Las referencias estéreo se promediaron a mono. Valores de referencia: mediana del conjunto de cada instrumento.

| Instrumento | T95 RC9 / referencia, ms | Centroide RC9 / referencia, Hz |
|---|---:|---:|
| 808 bombo | 116 / 243 | 49 / 53 |
| 808 caja | 60 / 39 | 2169 / 5759 |
| 808 hat abierto | 83 / 435 | 2560 / 8469 |
| 808 platillo | 108 / 877 | 2524 / 8680 |
| 909 hat cerrado | 16 / 65 | 2533 / 8487 |
| 909 ride | 132 / 498 | 3490 / 7666 |

Ver [datos completos](ryuji-quality/metrics.csv), [resumen](ryuji-quality/summary.json) y [espectros](ryuji-quality/spectral-comparison.png). Las medidas no sustituyen evaluación auditiva. La exportación aislada tampoco representa todos los efectos posibles de una sesión completa.

## Referencias de arquitectura

- [Analog Rytm MKII, manual oficial](https://www.elektron.se/wp-content/uploads/2025/01/Analog-Rytm-MKII-User-Manual_ENG_OS1.72_250130.pdf): generadores analógicos específicos y reproducción de muestras por voz. Referencia para combinar cuerpo sintetizado y textura, no para trasladar sus recursos de hardware al FM1.
- [Roland TR-1000](https://www.roland.com/us/products/tr-1000/): circuitos analógicos, ACB, otras formas de síntesis y sampling. Apoya una arquitectura por instrumento y capas.
- [Digitakt, contenido oficial](https://support.elektron.se/support/solutions/articles/43000570070-digitakt-missing-factory-content-): muestras de fábrica de 16 bits, 48 kHz, mono. Referencia para una fuente consistente y edición posterior.
- [Roland TR-909](https://www.roland.com/US/products/rc_tr-909/): la propia máquina combina percusión analógica con platillos muestreados. Un híbrido también tiene sentido histórico.

## Prototipo VA y límites

Se renderizaron fuera del firmware los modelos de bombo, caja y hat de [Mutable Instruments Plaits](https://github.com/pichenettes/eurorack/tree/master/plaits/dsp/drums), con licencia MIT. Se exploró una grilla gruesa de tone/decay y se seleccionó por bandas espectrales y T95. No se usó esa selección como puntuación de autenticidad.

El bombo seleccionado alcanza T95 de 250 ms frente a 243 ms de referencia, y centroide de 52.3 frente a 52.8 Hz. La caja todavía difiere espectralmente. El hat alcanza 291 ms frente a 435 ms y es demasiado brillante: 11.8 kHz frente a 8.47 kHz. Por tanto, el prototipo demuestra una dirección viable para investigar, no un reemplazo terminado.

Los tamaños de estado medidos en host fueron 72/204/80 bytes para bombo/caja/hat. No son mediciones de CPU ni ABI del FM1. El código flotante necesita evaluación y posiblemente adaptación a punto fijo antes de integrarlo. Cualquier reutilización debe conservar los avisos MIT.

## Memoria y formatos

El banco actual ocupa 219828 bytes; quedan 58660 bytes en la imagen RC9. Reemplazar el banco permite aproximadamente 278488 bytes para fuentes nuevas Y código adicional. Convertir las duraciones actuales completas a PCM16/44.1 requeriría 941480 bytes: no entra.

Como presupuesto ilustrativo, cuatro capturas 909 seleccionadas (hat cerrado, abierto, ride, crash), recortadas por energía, ocupan 243584 bytes en PCM16/44.1. Quedarían aproximadamente 34904 bytes para código adicional y otros datos. Esto no garantiza viabilidad: falta compilación objetivo, CPU con polifonía y efectos, y evaluación del recorte de colas. La RAM general de RC9 está cerca del límite.

La prueba de formatos produjo SNR mediana de 32.1 dB para PCM8 lineal y 80.4 dB para PCM16. µ-law8 alcanzó 37.8 dB. Una implementación exploratoria IMA4 en bloques de 64 dio 14.2 dB de mediana, especialmente mala en material agudo: no se recomienda adoptarla a partir de esta prueba. Estos valores describen los clips y codecs probados, no todos los codecs posibles.

## Propuesta de implementación

1. Priorizar 808/909 y mantener kits híbridos editables y random con locks.
2. Sustituir las fórmulas de bombo/caja/tom por modelos específicos de excitación, resonancia y ruido filtrado. VA no significa una misma senoide con diferentes presets.
3. Usar PCM de calidad y con licencia de redistribución para hats/ride/crash 909; evaluar el metal 808 con modelo de circuito y comparación auditiva antes de decidir VA o PCM.
4. Hacer que el preset inicial reproduzca limpio. Drive, fold, reducción de bits y filtros deben ser decisiones explícitas.
5. Separar envolvente de amplitud, envolvente tonal y afinación. En PCM, una envolvente puede acortar o atenuar, pero no inventar una cola inexistente mediante un bucle tonal.
6. Reducir variantes redundantes antes de sacrificar la calidad de todas las fuentes. Añadir pocos acentos LM/727 sólo cuando haya una fuente convincente y presupuesto.
7. Validar A/B a nivel comparable, colas, choke, afinación, polifonía y CPU en el FM1 antes de publicar otra versión.

## Reproducción y archivos locales

`tools/ryuji_quality_audit.c` incluye el harness de host y exporta 42 WAV a `build/quality`. Compilar con clang, los includes `firmware/src`, `build/gen`, y `-DDROPLETS_PARTICLES`, enlazando `-lm`; crear previamente el directorio de salida.

El directorio de trabajo hermano `work/ryuji-quality` conserva `analyze.py`, `format_study.py`, `va_probe.cc`, `select_va.py`, las capturas y los audios A/B. Los análisis usan numpy/scipy/soundfile/matplotlib. Los audios de referencia no se incluyen en el repositorio: el texto de descarga permite uso, pero no establece claramente permiso para redistribuir las muestras como banco de firmware.

`808-rc9-va-reference-private.wav`: tres grupos, cada uno RC9 → VA → referencia; bombo a 0 s, caja a 6.75 s, hat abierto a 13.5 s. Cada clip ocupa 2 s más 0.25 s de separación. Nivel ajustado por RMS de los primeros 250 ms, con límite de pico; no compara volumen original. Las colas de la audición están limitadas a 2 s; las métricas de referencia usan el archivo completo.
