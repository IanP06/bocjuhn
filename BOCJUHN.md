# bocjuhn 1.0 RC3 — Polvo / DROPLETS

Release identity: FM-1_197100, version B103. Built for the FM1 using the existing
TONITOR/Felucca update protocol and unchanged low-level boot, USB and flash code.
The historical internal identifiers remain where changing them would break stored data.

## Instrument and interface

The splash displays exactly `bocjuhn`, `created by ryuji kanagawa`, and
`argentina, 2026`. Polvo is sparse, static RGB dust on black, with a muted track
palette. Rendering uses the original band buffer, with no full framebuffer or
animated particle workload. BALLSY keeps its original 228 × 204 field at (6,30),
segmented walls, ball radius and single header. Only its colors change; there is
no decorative dust in its playing field.

The normal interface remains the original track screen. Turning a macro shows
its large temporary dial, then returns to that screen. There is no permanent
four-macro dashboard.

## Controls retained and added

EDIT OFF: K1 FILTER, K2 GRAINS, K3 CRUSH, K4 REVERB SEND.
SHIFT+K1 opens FILTER: K1 cutoff, K2 resonance, K3 DJ/LP/HP/BP; SHIFT+K1 exits.
Resonance AUTO preserves the former resonance curve. Values 0–100 set resonance
independently. DJ preserves the bipolar low-pass/high-pass control and center
bypass; fixed types map the full knob range to approximately 16–20,000 Hz.
There is one state-variable filter stage: LP/HP 12 dB/oct, BP 6 dB/oct per side.
No extra 24 dB stage or additional DSP chain is instantiated.

SHIFT+K2 retains the GRAINS editor (size, density, scatter, feedback).
SHIFT+K3 retains direct distortion, SHIFT+K4 direct delay send. Existing effect
pages remain. The expanded menu system and additional eight-parameter engine
menus are deferred in favor of the approved stability-first scope.

## DROPLETS

DROPLETS occupies CALIGULA's engine slot. It is an original, fixed-point engine
inspired by the short impulses and resonances the user likes in Plaits Particles;
it is not a Plaits port and does not incorporate its source code or diffuser.
Eight bounded resonant impulses share each track, with immediate note attacks,
held-chord pitch selection, noisy clicks, a second harmonic, and gentle descending
resonant attacks for water drops. No floating point or audio-thread allocation.

EDIT: DENSITY, COLOR, RESONANCE, SPREAD. DENSITY at minimum suppresses automatic
retriggering; played notes still attack immediately. RESONANCE extends the
ringing impulse. SPREAD ranges from a stable pitched center to dispersed pitches.
Factory families: CLICK, DROP, GLASS, RAIN, POOL, METAL, CHIME, DEEP, SPARK, MIST.
The native layer ADSR is preserved. For long single-hit ringing use RESONANCE;
an amplitude envelope cannot recreate a resonator that has already decayed.

## Existing RYUJI and FRACT work

PULSE remains renamed RYUJI; its first factory preset is KANAGAWA. Its macros are
STRUCTURE, DAMPING, BRIGHTNESS and POSITION. Its sustained body supports custom
ADSR, and its octave register affects keyboard, held voices and BALLSY. The
previous RYUJI regression suite remains required. FRACT and the bit-identical
neutral BLIPS reference remain preserved.

## Stability and limitations

New voice starts reserve a conservative measured cost before a burst can fill
the shared pool. Under budget pressure the scheduler reuses a compatible release
tail or declines a new start; note-offs still run. It does not impose a fixed
BALLSY count, ball count or smaller voice pool. Already fading voices are still
counted as work. The reactive guard may retire an older held, unlatched voice
after approximately 46 ms rather than waiting 1.5 seconds during an overrun.
An emergency note-off bitmap preserves releases when the main event ring fills.
Static filter coefficients are reused; GRAINS redraws only when its view changes.
Replacing CALIGULA removes 9,216 bytes of per-track delay-line storage.

This is bounded degradation, not unlimited polyphony. Host tests verify the
admission algorithm using a cost model, not physical CPU timing. USB, display,
both-core scheduling and worst-case DSP combinations still require a real FM1
stress test. No claim of guaranteed glitch-free operation is made before that.

## Storage and installation

INT2 format 32770 adds filter settings to tracks and scenes. Reading format 32769
retains earlier INTERVAL session contents and defaults to the original DJ filter.
CALIGULA designs regenerate as DROPLETS from their stored seed/family. They
cannot reproduce the old CALIGULA sound. General track effects remain session
state, matching the existing preset workflow. The original TONITOR namespace
remains separate. Keep an earlier memory backup for rollback; new sessions are
not promised to load correctly in older firmware.

See INSTALL-ES.txt for installation and recovery. The release includes a firmware
package, original TONITOR rollback, corresponding source, checksums, test logs and
host-rendered audio examples. Physical FM1 validation remains pending.
