# Repository guidance

## Firmware Hub description

- Keep `fmhub.json` → `what` as an enduring introduction to bocjuhn: what the instrument is, its main capabilities, and what musicians can do with it.
- Write in English, in plain language, within the Hub's 300-character limit. Prioritize six-track sound exploration and live layering, the range of eight engines, sound shaping, free loops, sequencing, accompaniment, and snapshot morphing.
- Do not replace the description with a release changelog when publishing updates. Release numbers, recent fixes, DSP/RAM implementation details, and version-specific validation status belong in release notes and the README's release section.
- Update the card only when the instrument's overall capabilities or identity change; preserve its introductory purpose. Keep hardware-validation information accurate in the release documentation.
- Release preparation scripts must preserve `what` instead of generating it from the latest release notes. Review the `fmhub.json` diff before publishing.
