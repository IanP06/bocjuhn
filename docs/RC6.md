# bocjuhn 1.0 RC6 — Living instruments

Visual release for the M-VAVE FM1. Sound engines, button/encoder assignments,
page traversal and the RC5 session format (32771) are unchanged.

- Eight engine-specific organisms on the main screen. DROPLETS is a field of
  small, independently trembling particles; activity follows the existing track
  output meter. These are abstract visualizations, not oscilloscopes.
- Engine colors follow the sound across tracks, meters, dials and track pages:
  FM blue; ACID green; DROPLETS cyan; FRACT magenta; DRUID lime; RYUJI red;
  HIVE yellow; OBRADOR orange. Recording and warning colors keep their meanings.
- Clean black performance screens. Engine name appears once on HOME; track,
  preset, octave, battery, chord/variation state, messages, meter and pan remain.
- Fine segmented temporary dials with a large value and the original units,
  bipolar center, OFF/AS DRAWN semantics and one-second return behavior.
- ACID's four parameter rows retain their assignments, with K1–K4 labels,
  aligned values and small amount bars. Other menus retain their navigation.
- BALLSY keeps its field, walls and ball geometry; colors follow the engine.

The display uses the existing band buffer. Only the 128 × 112 organism area
animates, at most once every 83 ms; it stops after the meter decays to silence.
Rendering uses bounded integer geometry and existing sine tables, with no
allocation, FFT, extra audio work or full framebuffer. Performance on the actual
device still needs validation; host/WASM tests do not measure hardware timing.

## Release identity and compatibility

- Product: `FM-1_197100`; device version: `B106`.
- Release package: `bocjuhn-1.0-rc6.fwsc`, including the update loader.
- RC5 sessions remain compatible. Older ANIMA sessions follow RC5's migration
  to ACID. Other firmware projects/samples are not promised to be compatible.
- Export a physical-device backup before changing firmware. Browser simulator
  memory is separate from the FM1's memory.

## Physical validation: pending

This release has not yet been validated on a physical FM1. Earlier RC1 was
confirmed by the owner, which does not validate RC6. Do not report the Firmware
Hub hardware requirements as passed until the checks in HARDWARE-TEST.md are
completed with this exact package.

## Credits

bocjuhn by ryuji kanagawa, based on Tonitor by Nuno Zimas, with the Felucca/SLOOP
update path and the upstream credits and licenses preserved. The new visuals
are original C drawings inspired by the organism concept in JIANT and the
instrument graphics of OP-1/OP-XY. No graphics or code from those references
were copied. See LICENSING.md and LICENSES for component licenses.
