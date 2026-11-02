# bocjuhn — M-VAVE FM1 firmware

by **ryuji kanagawa** · Argentina · 2026

Six tracks for sound exploration, free loops and clocked performance. Eight
engines: FM, ACID, DROPLETS, FRACT, DRUID, RYUJI, HIVE and OBRADOR. Granular
processing, filters, modulation, effects, BALLSY, BRICKLAYER and PLAY ALONG.

Each engine has its own animated organism and saturated color on black.
Play, layer, loop and transform sounds through the FM1's physical controls
or try the complete instrument in your browser.

![DROPLETS, actual RC7 firmware screen](docs/card.png)

## Current release: 1.0 RC7 — Living instruments

Engine-specific animated organisms, clean black screens, saturated engine
colors across tracks and menus, and redesigned temporary dials. Sound engines,
controls and RC5 session format are unchanged. [Release notes](docs/RC7.md).

**RC7 has been tested on the owner's physical M-VAVE FM1 and is confirmed working**
by ryuji kanagawa on 2026-11-02. Build, host, browser and package checks
are also recorded with the release. [Hardware test record](docs/HARDWARE-TEST.md).

[Play in your browser — no FM1 needed](https://ianp06.github.io/bocjuhn/)
· [Download RC7](https://github.com/IanP06/bocjuhn/releases/tag/v1.0.0-rc7)
· [Controls](ACID-CLOCK-RC5.txt)
· [Installation and recovery](release/bocjuhn-1.0-rc7/INSTALL-ES.txt)

## Simulator and installer

The public player runs at **https://ianp06.github.io/bocjuhn/**. Press Power to
enable sound, then play the keys or use your computer keyboard. Controls opens
the guide; PRESETS changes engine and the six upper buttons select tracks.
No MIDI or USB device is required. Nothing is flashed by the player.
The simulator's saved memory stays in your browser. Use Export memory to back it up.

Install Node.js, clone this repository and run:

```sh
node tools/serve_bocjuhn.cjs
```

Open http://127.0.0.1:8767 for the simulator and
http://127.0.0.1:8767/install for the installer. The simulator executes the
firmware C code compiled to WebAssembly. It does not model physical CPU timing.
Reading identity does not flash a device; installation requires the browser's
explicit install action. Use Chrome or Edge for Web MIDI installation.

Back up the physical FM1 before switching firmware. Simulator Export memory
backs up only the browser. RC7 reads RC5 sessions (32771); unrelated firmware
projects and samples are not guaranteed compatible and can be overwritten.
Earlier ANIMA sessions migrate to ACID as in RC5. See the installation guide.

## Build

Linux x86-64, Python 3 with Pillow, clang, Emscripten and Node.js are needed for
the complete software checks. The target uses the JieLi pi32v2 toolchain.
The supplied Dockerfile provides the host tools:

```sh
docker build -t bocjuhn-host -f tests/Dockerfile.host .
docker run --rm -v "$PWD:/work" bocjuhn-host sh tools/prepare_target.sh
docker run --rm -v "$PWD:/work" bocjuhn-host sh tests/living.sh
```

The test/build script expects the 20260730.1 toolchain under build/deps/jieli;
set JIELI_TOOLCHAIN in the script if using a different installation. The release
builder is tools/build_living_release.py; output is build/rc7. Build metadata
hashes the firmware and generated headers. Product FM-1_197100, version B107.
The .fwsc includes the existing update loader; no separate data flash is needed.

## Firmware Hub

The card is in [fmhub.json](fmhub.json). The submission text is prepared in
[docs/HUB-SUBMISSION.txt](docs/HUB-SUBMISSION.txt), **not sent**. Real-device operation is confirmed by the owner. The hardware test record
distinguishes that confirmation from individually documented installation and exit checks.

## License and credits

Firmware and new code: GPL-3.0-only. See [LICENSE](LICENSE),
[LICENSING.md](LICENSING.md), [LICENSES](LICENSES) and
[the original Tonitor README](README-TONITOR.md).
Based on [Tonitor](https://github.com/nzimas/tonitor) by Nuno Zimas,
which builds on Felucca and SLOOP. Their credits and licenses are retained.
Additional third-party components retain their licenses, including Plaits,
Rings, msfa and the bundled fonts. The new visual drawings are original,
inspired by JIANT's organisms and OP-1/OP-XY's instrument graphics.

This is unofficial community firmware, provided without warranty.
