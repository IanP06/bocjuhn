# bocjuhn 1.0 RC9 — Hybrid kits / pinned guide / four-pole filters

RYUJI concentrates on 808 and 909: 48 original percussion sources, comprising the existing 16, fourteen 808-inspired sources, ten 909-inspired sources, four LM/LinnDrum accents and four 727 percussion accents. All audio is synthesized offline from original code; no factory ROMs or third-party recordings are included. LM/LinnDrum and 727 textures are synthetic interpretations, not authentic sampled acoustic kits or exact circuit emulations.

## Playing and editing kits

On RYUJI, hold the black EDIT key for 600 ms to open KIT, with BALLSY off. Hold it again to close. Existing SHIFT + EDIT still controls BALLSY; holding EDIT with BALLSY enabled retains its existing page shortcut.

- K1: select pad 1–16. Playing a white key selects and auditions that pad too.
- K2: choose a source compatible with the pad's instrument role. The screen shows the family and actual source name.
- K3: tune that pad from -24 to +24 semitones, independently of track octave.
- K4: clockwise locks the pad; counterclockwise unlocks it.
- HOME in KIT: randomize all unlocked sources.
- SHIFT + HOME in KIT: randomize only the selected pad, if unlocked.

Randomization gives each 808/909 source four times the weight of an original or accent source, within the same pad role. It preserves tuning and never substitutes a kick with a hat. Locked pads retain both source and tuning; manual editing is still possible. Repeating the same random seed produces the same source map. Saving stores the resulting map, so playback does not rerandomize it. HOME outside KIT remains the existing new-sound operation and can replace the entire kit.

The closed hat on pad 5 still chokes the open hat on pad 6. Pad identity remains independent of track transposition. Source selection and tuning are captured per voice at note-on; whole-design edits use the engine's existing publication/crossfade path.

## Collections

Unsaved factory slots 1–6 retain the original collections for compatibility. Slots 7–8 are 808 MIX and 909 MIX. These are full playable kits: where a family lacks an instrument, the original source fills that pad.

Slots 9–12 are NIGHT, WAREHOUSE, LM ACCENT and LATIN GRID hybrids, all built around 808/909. Slots 13–24 are seed-varied HYBRID kits with the same 808/909 weighting. Previously saved user presets take precedence over factory defaults. All 64 RYUJI preset slots can store edited maps, tuning and locks.

| Family | New sources | Intended character |
|---|---:|---|
| 808 | 14 | Sine-based low drums, noise snare/clap, metallic hats, cowbell and conga |
| 909 | 10 | Driven kick, harder snare, hats, tom, ride/crash textures |
| LM / LinnDrum accents | 4 | Kick, snare, clap and sidestick with quantized transients |
| 727 accents | 4 | Bongo, conga, agogo and cabasa |

606, 707 and CR-78 were researched but are not included in this focused release. The 808/909 sources use 22050 Hz throughout, with longer low drums (550 ms), cymbal tails (650 ms) and open hats (350 ms). RYUJI plays each source once; ADSR shapes its amplitude without looping fragments. Long multisampled cymbal decays, velocity layers and user WAV import are not included.

## RYUJI envelope correction

Previously enabling the track ADSR crossfaded the sample into a repeated 256/512-sample region. That periodic loop could introduce a pitched tone absent from the original percussion. RC9 removes that sustain loop and uses the same finite PCM playback, native decay, pitch and processing whether ADSR is enabled or disabled. The track ADSR only applies amplitude shaping afterward.

Sustain and release cannot extend audio beyond the finite sample tail. A long attack can suppress a short percussion transient; it does not time-stretch the sample. Intentional source tuning and STRUCTURE/POSITION processing can still change timbre. Offline generation now reapplies the endpoint fade after DC removal, ensuring the first and final PCM values are zero.

Regression coverage compares every output sample with internal hold enabled/disabled across all 48 sources, including POSITION processing. Outputs are identical before the external amplitude envelope; all held sources finish naturally. A complete-mixer test verifies that maximum ADSR sustain does not leave a pad oscillating indefinitely.

## Accompaniment and filters

REACT, SHADOW and ARPEGGIO retain their guide when selecting another track. SHIFT + track inside PLAY ALONG explicitly changes it. Guide choice persists in sessions and snapshots.

FILTER K3 now offers DJ, LP/HP/BP 12 and LP/HP/BP 24. Manual resonance spans Q approximately 0.707–12; AUTO retains its old range up to Q4. Four-pole variants cascade two SVF sections; they are not ladder/MS-20 models. LP/HP slopes are nominally 24 dB/octave, with approximately 12 dB/octave on each skirt for the four-pole BP. Existing explicit resonance settings sound stronger. The output limiter can contain resonant peaks at high signal levels.

## Persistence and compatibility

Session format 32774 reads previous RC7/RC8/early-RC9 sessions. Kit maps for dormant RYUJI engines also persist. Snapshots distinguish kits even when their original sound seed is identical; source maps switch with the sound at the morph transition. Track copy includes the kit through the unchanged design layout.

RYUJI preset metadata and kit maps are saved together in a dedicated atomic flash record (number 1002). Legacy presets in record 1001 remain readable until migrated. Ordinary sessions store only used snapshot kit entries; the host storage regression still saves 64 small sessions before FULL. Larger loop/snapshot sessions consume more space.

Export a backup before upgrading. Older firmware does not understand the new session/RYUJI preset extension. Browser memory and physical FM1 memory are separate backups.

## Validation and limits

Host tests cover all 48 sources at -24/0/+24 tuning; bounded non-silent output; deterministic role-aware randomization; all-locked and partially locked kits; preset/session/snapshot round trips; distinct maps sharing a seed; dormant-engine storage; editor navigation; the existing transport, loops, presets, snapshots, modulation and FX regressions. WebAssembly tests cover audio, kit controls, page navigation and backup. The target build checks image, RAM and pool limits.

Physical FM1 validation remains pending. Host timing does not establish device CPU headroom. Final memory counts and package hash are in the release manifest. Build with `sh tests/ryuji_kits.sh` in the host container.

`ryuji-families.wav` auditions four strikes per family: 0–4 s 808, 4–8 s 909, 8–12 s LM accents, 12–16 s 727 percussion. It uses the C engine, not a physical FM1 recording.

## Research references

- [Roland TR-606 manual](https://cdn.roland.com/assets/media/pdf/TR-606_OM.pdf): limited analog drum palette.
- [Roland TR-707](https://www.roland.com/us/products/rc_tr-707/): crisp, punchy sampled-drum reference.
- [Roland TR-727 manual](https://cdn.roland.com/assets/media/pdf/TR-727_OM.pdf): Latin percussion roles.
- [Roland CR-78 instrument list](https://static.roland.com/assets/media/pdf/V71_DataList_eng01_W.pdf): bongo, conga, cowbell, claves, guiro, maracas and metallic percussion.
- [Roger Linn museum](https://www.rogerlinndesign.com/about/about-museum): distinction between LM-1 and LinnDrum, including the latter's expanded sample palette.

These references informed instrument selection and sound-design goals. The generator is an original approximation, not a transcription of their circuitry or sample data.
