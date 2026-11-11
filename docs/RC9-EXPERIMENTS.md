> Historical design notes from the filter-only candidate. RYUJI is now implemented; see [RC9](RC9.md) for current behavior and validation.

# RC9 development candidate

Local development branch: `codex/accompaniment-filters-rc9`. Public RC8 remains the published release. RYUJI bank changes are proposals, not implemented. Hardware validation pending.

## Accompaniment

REACT, SHADOW and ARPEGGIO pin the selected guide when enabled. Selecting another track no longer resets history, cancels the accompaniment or changes the input listener. Within PLAY ALONG, SHIFT + track explicitly changes the guide. The guide LED follows the pinned guide. Sessions and snapshots retain the guide independently of the selected track.

Session format 32773 appends the guide and snapshot guide indices; old RC7/RC8 sessions remain readable. Old firmware cannot be assumed to read new-format saves: retain an exported backup before testing.

## Filters

Filter page: K1 cutoff, K2 resonance, K3 type. Types: DJ, LP 12, HP 12, BP 12, LP 24, HP 24, BP 24. Explicit resonance now spans Q approximately 0.707–12; AUTO retains the original range up to Q4. Existing saves with explicit resonance therefore sound more resonant at the same setting.

Four-pole variants cascade the existing TPT SVF with another section at Q0.707. They are not ladder or MS-20 emulations. LP/HP have nominal 24 dB/octave slopes; the four-pole BP has approximately 12 dB/octave on each skirt. State resets when entering a new topology. Existing output limiting remains in place and can reduce the perceived resonance boost at high levels.

At 1 kHz cutoff, isolated DSP measurements: LP at 8 kHz changes from -38.13 to -65.88 dB; HP at 125 Hz from -36.16 to -59.11 dB. Q4 to Q12 increases the isolated peak by 9.53 dB. Six-track host filter-only benchmark: about 0.053 seconds for two poles versus 0.079 for four poles. This is neither overall CPU usage nor a timing guarantee for FM1 hardware.

Target build: image 344764/581564 bytes; RAM 97236/98304; pool 327024/344064. Relative to RC8: +1280 image, +32 RAM, +144 audio pool. General RAM headroom is only 1068 bytes. CPU headroom requires a real-device stress test with six tracks and effects.

Validation: guide navigation for all three modes; explicit guide reassignment; guide persistence through session and snapshot; migration from RC8; filter response and bounded state sweeps; six-track complete-mixer cutoff sweeps; existing host regressions for interval, flows, latch, playalong, loopmodes, looptie, tracktransp, setup, backup, snapshots, sessions, modulation; FX/CLOCK coverage for gate ownership, mixed grids, effects and PRISM. Test source: tests/host/accompaniment_filter_test.c.

## Architecture choices to discuss

- MS-20/K35-inspired: nonlinear two-pole LP/HP, aggressive feedback and input drive. First choice for a cutting sound; needs a measured prototype.
- Ladder: four-pole low-pass with feedback saturation and optional bass compensation. Richer nonlinear processing costs more than the current cascade.
- LPG: coupled amplitude and low-pass response with decay memory. Suits woody percussive articulation rather than sharp resonant sweeps.

References: [Korg FilterArk manual](https://storage.korg.com/korgms/support/Filter_Ark/Filter_Ark_OM_E.pdf), [The Art of VA Filter Design](https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.0.0a.pdf), [DAFx ladder filter paper](https://dafx.de/paper-archive/2004/P_061.PDF).

## RYUJI proposal — no engine change yet

Current six collections DRY, METAL, DUST, RUBBER, LOGIC and VOID share the same sixteen original synthetic PCM sources. Their tone, rate, fold and decay differ. The source bank occupies 46200 bytes (8-bit mono at 22050 Hz, excluding small metadata).

Recommended direction: preserve the current bank; add original 808-inspired and 909-inspired families and an original or appropriately licensed acoustic LM-style family. Offer curated hybrid kits and per-pad source selection from that library. Save the kit map with the preset/session. Keep existing global shaping controls.

A role-aware randomizer should choose kicks for kick pads, hats for hat pads, etc.; use a saved deterministic seed and individual pad locks. Randomization occurs on an explicit action, not on each note. Custom kits initially mean selecting supplied sources per pad. Importing user WAV files is a separate storage/tooling feature.

Three additional banks at the current bank size cost roughly 138600 bytes, leaving about 98200 bytes of image capacity before new code. Long cymbals and more velocity layers increase this substantially. Pre-rendered percussion keeps runtime playback close to the existing design, but is not a full circuit model.

Options: (1) fixed classic kits, smallest interaction change; (2) curated hybrid kits plus per-pad source selection, recommended; (3) add seeded randomization and locks to option 2. These can be combined incrementally once the desired workflow is agreed.
