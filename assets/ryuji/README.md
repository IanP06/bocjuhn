# RYUJI sample assets

These are prepared mono PCM16 source files, not the final firmware storage format. `tools/gen_pulse.py` deterministically compands them for playback. `manifest.json` records exact source repositories, revisions, input/output SHA-256, rates and edits.

- Michael Fischer / Technopolis, TR-808 Sound Sample Set (1994), via tidalcycles/sounds-tr808-fischer, CC0-1.0. See fischer-README.md and fischer-LICENSE.txt.
- Matthew Cieplak, ER-99, GPL-3.0. See er99-README.md and er99-LICENSE.txt. Original capture provenance is not documented upstream; do not advertise these as official Roland ROMs.
- Emilie Gillet, Plaits bass drum model, MIT. Its fixed-point topology adaptation is in firmware/src/ryuji_va.h; see Mutable-Instruments-LICENSE.txt.

The sample bank is distributed with complete corresponding firmware and build sources. No reference recordings from the private quality audit are included. Roland, TR-808 and TR-909 identify sonic references, with no affiliation or endorsement implied.
