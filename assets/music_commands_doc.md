# Music Commands Documentation

| Command | Function | Parameters | Parameter Bytes | Total Bytes |
|---------|----------|------------|-----------------|-------------|
| $D0 | Music_Octave8 | 0 | - | 1 |
| $D1 | Music_Octave7 | 0 | - | 1 |
| $D2 | Music_Octave6 | 0 | - | 1 |
| $D3 | Music_Octave5 | 0 | - | 1 |
| $D4 | Music_Octave4 | 0 | - | 1 |
| $D5 | Music_Octave3 | 0 | - | 1 |
| $D6 | Music_Octave2 | 0 | - | 1 |
| $D7 | Music_Octave1 | 0 | - | 1 |
| $D8 | Music_NoteType | 2 | 1, 1 | 3 |
| $D9 | Music_ForceOctave | 1 | 1 | 2 |
| $DA | Music_Tempo | 2 | 1, 1 | 3 |
| $DB | Music_DutyCycle | 1 | 1 | 2 |
| $DC | Music_Intensity | 1 | 1 | 2 |
| $DD | Music_SoundStatus | 1 | 1 | 2 |
| $DE | Music_SoundDuty | 1 | 1 | 2 |
| $DF | Music_ToggleSFX | 0 | - | 1 |
| $E0 | Music_SlidePitchTo | 2 | 1, 1 | 3 |
| $E1 | Music_Vibrato | 2 | 1, 1 | 3 |
| $E2 | MusicE2 (unused) | 1 | 1 | 2 |
| $E3 | Music_ToggleNoise | 1 | 1 | 2 |
| $E4 | Music_Panning | 1 | 1 | 2 |
| $E5 | Music_Volume | 1 | 1 | 2 |
| $E6 | Music_Tone | 2 | 1, 1 | 3 |
| $E7 | MusicE7 (unused) | 1 | 1 | 2 |
| $E8 | MusicE8 (unused) | 1 | 1 | 2 |
| $E9 | Music_TempoRelative | 1 | 1 | 2 |
| $EA | Music_RestartChannel | 2 | 1, 1 | 3 |
| $EB | Music_NewSong | 2 | 1, 1 | 3 |
| $EC | Music_SFXPriorityOn | 0 | - | 1 |
| $ED | Music_SFXPriorityOff | 0 | - | 1 |
| $EE | MusicEE (conditional jump) | 2 | 1, 1 | 3 |
| $EF | Music_StereoPanning | 1 | 1 | 2 |
| $F0 | Music_SFXToggleNoise | 1 | 1 | 2 |
| $F1 | MusicF1 (nothing) | 0 | - | 1 |
| $F2 | MusicF2 (nothing) | 0 | - | 1 |
| $F3 | MusicF3 (nothing) | 0 | - | 1 |
| $F4 | MusicF4 (nothing) | 0 | - | 1 |
| $F5 | MusicF5 (nothing) | 0 | - | 1 |
| $F6 | MusicF6 (nothing) | 0 | - | 1 |
| $F7 | MusicF7 (nothing) | 0 | - | 1 |
| $F8 | MusicF8 (nothing) | 0 | - | 1 |
| $F9 | MusicF9 (unused) | 0 | - | 1 |
| $FA | Music_SetCondition | 1 | 1 | 2 |
| $FB | Music_JumpIf | 3 | 1, 1, 1 | 4 |
| $FC | Music_JumpChannel | 2 | 1, 1 | 3 |
| $FD | Music_LoopChannel | 3 | 1, 1, 1 | 4 |
| $FE | Music_CallChannel | 2 | 1, 1 | 3 |
| $FF | Music_EndChannel | 0 | - | 1 |

## Notes:
- All address parameters (marked as "ll hh" in comments) are 2-byte little-endian values, counted as two 1-byte parameters
- Single-byte parameters include counters, flags, and immediate values
- Commands $F1-$F8 do nothing and return immediately
- Commands $E2, $E7, $E8 are marked as unused but still process their parameters
- Command $F9 is unused but sets a flag without reading parameters