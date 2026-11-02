# bocjuhn 1.0 RC7 — Moving instruments

Eight living engine visualizations, saturated accents and a black performance screen.

- FM: six independently orbiting operators connected by open, bending curves.
- DROPLETS: small trembling particles with individual drift, responsive to quiet signals.
- DRUID: five flowing, interlacing resonant ribbons.
- HIVE: thirteen drifting cells with independent orbits.
- ACID, FRACT, RYUJI and OBRADOR: dynamic motion follows track activity.
- Continuous dials with a luminous endpoint, separate short needle and clear label spacing.
- Rajdhani Medium/SemiBold interface typography; the bocjuhn signature is preserved.
- BALLSY retains its six-color ball palette. Engine accents remain on the frame and menus.

Visualization activity uses the existing output meter with logarithmic sensitivity.
These abstract organisms are not oscilloscopes or FM algorithm diagrams.
The display uses the existing band buffer, bounded integer geometry and sine tables.
Animation updates at most once per 83 ms and stops after activity decays to silence.
There is no heap allocation, FFT, full framebuffer or additional audio processing.

Sound engines, controls, navigation, one-second dial return and session format 32771
are preserved. Browser memory and physical FM1 memory are separate.

## Package

- Product: `FM-1_197100`; device version: `B107`.
- Package: `bocjuhn-1.0-rc7.fwsc`, including the update loader.
- Source, software checks, hashes and installation instructions accompany the release.

## Physical validation: confirmed by the owner

On 2026-11-02, ryuji kanagawa confirmed that RC7 was tested on their own physical
M-VAVE FM-1 and that everything works correctly. This is a real-device owner
confirmation, in addition to the recorded software checks.

The confirmation covers overall operation; individual installation routes and
interrupted-update recovery were not separately documented. See HARDWARE-TEST.md.

## Credits

bocjuhn by ryuji kanagawa, based on Tonitor by Nuno Zimas. Upstream credits and
licenses are preserved. Visual inspiration: JIANT's organism concept and the instrument
graphics of OP-1/OP-XY. Drawings are original C geometry; reference graphics and code
were not copied. Rajdhani is bundled under SIL OFL 1.1. See LICENSING.md.
