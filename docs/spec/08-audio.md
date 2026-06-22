# 08 — Audio (frontend/audio.c, sound, VAGS\BANK1.SBK) — spec

Our-words spec from decompiled Sound_Init/Play_Sound @0x415cd8/Modify_Sound/Kill_Sound/Sound_Stop +
Load_Game_Vags. Original uses DirectSound; the port shims to WebAudio.

## Sound bank
- VAGS\BANK1.SBK = sound bank loaded by Load_Game_Vags at boot. Samples are 8-bit unsigned PCM (PC port
  of PSX VAG ADPCM, pre-decoded), played at a base rate (~11025 Hz) with per-play frequency scaling.
- A sound TABLE indexes the bank: each entry is 0x1c bytes — fields used by Play_Sound: +4 buffer/sample
  ref, +8 loop flag, +0xc freq, +0x10 vol, +4/+8.. pan/range. Sounds: engine loop, tyre scrape/skid,
  collisions/crashes, UI clicks, countdown beeps, etc.

## Mixer / channels
- 4 SFX channels (index 0..3). Play_Sound(ch, table, idx, freq, _, vol): if ch==-1 pick a free channel
  (FUN_416494); Kill_Sound(ch); create/assign a DirectSound buffer for the sample; set play freq
  (×2 scaling), volume, pan; Play (looping if entry loop flag). Per-channel state @0x716d20 (stride 0x14):
  {buffer ptr, vol, pan, freq, base}.
- Modify_Sound(ch, freq, vol/pan, …): live-update a playing channel — used for the ENGINE sound (continuously
  set pitch/volume from the player car's RPM/speed) and for distance-attenuated positional SFX.
- Sound_Stop on race exit; Sound_Init/Sound_Remove lifecycle; global volume from _sound_volume.
- CD audio (Start_CD_Audio / Read_CD_Toc_) plays the music tracks (the "CD Audio Player" menu lists named
  tracks) — separate from SFX; for the port, optional/absent (CD tracks aren't in Dirinfo).

## Port mapping (WebAudio — already implemented, matches this model)
- Decode BANK1.SBK samples to AudioBuffers once (8-bit unsigned → float).
- Engine: one looping BufferSource + GainNode; each frame set playbackRate from RPM (≈ Modify_Sound pitch)
  and gain from throttle/speed. (audio_engine in the port.)
- One-shots: transient BufferSource+Gain per event (collisions, clicks, skids). (audio_oneshot.)
- 4-channel limit isn't required in WebAudio (unlimited voices) but pitch/volume mapping should follow the
  per-event freq/vol from the sound table for fidelity.

## TODO from spec
- Read Load_Game_Vags for the exact SBK layout (sample directory: offsets/lengths/rates) so the sound
  table indices map to the right samples. Identify which table index = engine/skid/crash/UI to drive the
  WebAudio mapping faithfully (currently approximate). Per-car engine pitch curve from car_handling RPM.
