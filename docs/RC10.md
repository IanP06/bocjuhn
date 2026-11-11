# bocjuhn RC10 — RYUJI hybrid audio engine

RC10 replaces the simplified 808/909 bank with a mixture of integer VA and recorded sources. It preserves the 48 source IDs, editable kit maps, tuning, random locks and session format 32774. Existing sessions load but RYUJI timbres and zero-macro behavior intentionally change.

## Sources and playback

- 808 kick/sub: pulse/diode exciter and two-integrator resonator, adapted to fixed point from the MIT-licensed Plaits topology. Tom/conga use a two-mode decaying oscillator.
- 808 snare, clap, hats, rim, claves, cowbell, cymbal and maracas: Michael Fischer / Technopolis captures, from the CC0 repository `tidalcycles/sounds-tr808-fischer`.
- 909 kick/sub, snare, clap, rim and tom: instrument-specific oscillator/noise models. These are compact approximations, not transistor-level models.
- 909 hats, ride and crash: the sample assets distributed with Matthew Cieplak's GPL-3.0 ER-99. The upstream project does not document the original recording chain. The closed hat is an amplitude-shaped derivative of its open hat sample.
- ORIG and the small LM/727 accent sets remain synthetic secondary sounds. They are not recordings of LinnDrum or TR-727 hardware.

Exact upstream commits, hashes, processing and licenses are in `assets/ryuji/manifest.json` and its adjacent notices. AudioRealism's restricted sample pack and RAUM45's audit references are NOT bundled.

The compiled sample bank uses G.711-style µ-law companding, decoded by a 512-byte table and interpolated in integer arithmetic. This is lossy 8-bit storage with nonlinear amplitude resolution, not PCM16 or lossless compression. It spends substantially more resolution on quiet tails than the old linear 8-bit bank. Sources remain PCM16 WAVs in the repository for reproducible builds and future format changes.

909 metal, the 808 snare and both 808 hats retain 44.1 kHz. The long 808 cymbal uses 32 kHz; shorter midrange 808 percussion uses 22.05 kHz. Secondary legacy sources use 11.025 kHz. Tails are trimmed only after 99.99% cumulative energy plus 15 ms, followed by an end fade, except the deliberately shortened 909 closed hat and compact secondary synthetic bank. No PCM looping or runtime decompression buffers are used.

## Controls

Fresh sessions start RYUJI on an 808/909 kit; HOME family draws favor the primary/hybrid collections. Explicit ORIG preset slots and saved kits remain available.

Kit controls remain: hold EDIT, K1 pad, K2 source, K3 tuning, K4 lock; HOME randomizes unlocked sources and SHIFT+HOME only the selected pad. Closed hat chokes open hat.

Zero macros now bypass coloring and additional amplitude decay. STRUCTURE above zero adds fold; DAMPING shortens PCM and changes VA decay; negative PCM damping cannot invent a longer recording. BRIGHTNESS below zero filters, above zero adds a gentle high shelf. POSITION uses the existing comb blend for PCM and a restrained level/timbre blend for VA. Track ADSR only shapes amplitude; it never turns a source into a pitched loop. VA voices have a finite safety tail and stop when their state becomes inaudible.

## Validation and limits

Tests check exact neutral PCM reproduction after decoder/interpolation, all 48 sources under ADSR hold on/off, tuning/register/macro extremes, finite VA tails, kit persistence, random locks, guide-track accompaniment, effects and the WebAssembly simulator. A separate undefined-behavior sanitizer stress test covers the VA math.

Target linker checks verify image, general RAM and pool limits. Host throughput comparisons indicate relative algorithmic cost only; they are not measurements of the FM1's CPU load. Physical six-track playback with effects, timing and listening validation remain pending. The release remains a prerelease.

Final build: image 569496 / 581564 bytes (12068 bytes free); .data+.bss 97252 / 98304; pool 330480 / 344064. Sample bank 264054 bytes. General RAM and pool allocations are unchanged from RC9. Per-voice RYUJI state is 120 bytes inside the existing larger voice union.

Capture companding SNR is measured separately from resampling: see `docs/ryuji-quality/rc10-audio-metrics.json`. The A/B audio uses RC9 then RC10 for each instrument, level matched over the first 250 ms; it contains only firmware renders, not the private reference recordings.
