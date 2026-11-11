# Replicant - Version History

Every version is published automatically as a GitHub Release with the plugin attached (from 11 November 2026), and the APT Industrial updater installs the newest one for you.

## 1.0.4 - 11 November 2026
- The version number is now shown in the plugin window. Click it to read this version history inside the plugin.

## 1.0.3 - 9 November 2026
- FIXED: a note-off sent at the very end of an audio block (or in an empty block) was lost, so the note hung until you pressed Stop. It is now always played.

## 1.0.2 - 9 November 2026
- Stop means silence. When a song is playing and you press Stop in FL Studio (or any DAW), the plugin fades out in about 15 ms and goes silent: every note, the sustain, any internal sequencer or auto mode, and delay and reverb tails. The same happens on MIDI All Notes Off / All Sound Off while stopped, on host reset, and when a project loads.
- Playing live while the transport is stopped is unaffected: notes and tails ring out naturally.
- The sustain pedal is released when you press Stop.

## 1.0.0 - 8 November 2026
- First version: a Yamaha CS-80 tribute by APT Industrial. Eight voices (up to 16) on two channels, with PWM oscillators, the 12 dB high-pass and low-pass filters, filter and amp envelopes, touch response, sub oscillator, ring modulator, polyphonic portamento and glissando, the ribbon, poly aftertouch and MPE, and analog drift.
- Chorus/tremolo, tempo-synced delay and hall; 22 tone-selector voices plus 20 film-score-era sounds and 4 memories; the ESPER neon-noir look.

