# DD2 decompile → port SPEC (methodology: spec from the complete decompile, then implement)

Source: re_out/dd2_decomp.c (44,417 lines, 497 named functions + FUN_ stubs) from dd2.exe.
Goal: faithful 1:1 spec of every module's behavior/data, in our own words, to implement the WASM port against.
Data: DestructionDerby2/Dirinfo (22MB archive, 114 files, identical native↔wasm). Dirs: LEV0 (front-end),
LEV1–LEVB (11 tracks), LEVF (results/screen), VAGS (audio). LEVC/D/E referenced in code, no data (cut).

## Original module structure (from embedded C:\Pcdd2\ source paths)
CORE:        main.C, game.C, initcode/init.C, compress.C, file.C, mem.C, object.C, draw.C, font.C, d.C, debug.C
PSX LIBS:    libgpu.c, libgte.c, gfx.c, ddraw.C   (GPU/GTE emulation + DirectDraw blit — THE software GTE)
GRAPHICS:    camera.C, scene.C, sky.C, drawcar.C, damage.C, dynamics.C, overlay.C, flag.C, lensflre.C, sparking.C
HANDLING:    car.C, handling.C, suspensn.C, collsn3d.C, carclsn.C, barrclsn.C, carstuff.C, ai.C, shadow.C, info.C, driver.c
FRONTEND:    frontend.c, init.c, loading.c, general.c, tracksel.c, carsel.c, racemode.c, racetype.c, raceover.c,
             config.c, credits.c, info.c, fastlaps.c, name.c, centresc.c, audio.c, cdaudio.c, pad/dpad/negcon/madcatz/padstuff.c
RACETYPE:    champ.c, mulchamp.c, racetype/misc.c
RESULTS:     results/card.c, results/champ.c, endofsea.c, results/init.c
CARD(save):  card/card.c, card/misc.c
MISC:        debris.C, denting.C, follow.C, movie.C, pause.C, pit.c, replay.C

## 11 tracks (authoritative, racing-name pointer array @0x4682f8, index = order):
0 Pine Hills Raceway, 1 Chalk Canyon, 2 S.C.A. Motorplex, 3 Caprio County Raceway, 4 Black Sail Valley,
5 Liberty City, 6 Ultimate Destruction, 7 Red Pike Arena, 8 The Colosseum, 9 The Pit, 10 Death Bowl.
NOTE: this menu/array order is NOT the LEVx dir order (e.g. LEV6=trees=Pine Hills, LEV5=desert=Caprio);
the menu-index→LEVx map must be read from the track-select code (tracksel.c) — TODO.
Modes (NOT tracks): Wrecking Racing, Stock Car, Destruction Derby, Practice, Time Trials, Total Destruction,
Multiplayer, Championship.

## Spec plan (one file per area; read decompiled fns in full, describe behavior + data):
01-data-io   (Dirinfo/file.C/compress.C/object.C/font.C, LEVEL.DAT/CLT/TDF/TX/SPR formats)  [partly in REVERSING.md]
02-gte-gpu   (libgte.c/libgpu.c: GTE math, display-list primitives, ordering-table, CLUT/tpage)  ← THE software GTE
03-gameloop  (main.C/game.C/init.C: states, per-frame order, level load dispatch by _current_level)
04-handling  (car.C/handling.C/suspensn.C/collsn3d.C/carclsn.C: fixed-point vehicle physics + collisions)
05-ai        (ai.C/follow.C: opponent driving)
06-scene     (scene.C/camera.C/drawcar.C/sky.C/damage.C/denting.C: in-race draw via GTE)
07-frontend  (frontend.c + menu modules: title/menu/tracksel/carsel/stats/results/championship flow)
08-audio     (frontend/audio.c + VAGS/BANK1.SBK playback)
