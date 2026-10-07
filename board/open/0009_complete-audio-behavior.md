Type: Work item
Title: Complete sound effects and Redbook selection

## Contract

Provide all reached gameplay/menu/commentary effects, pitch/spatial behavior, automatic game-state Redbook selection and persistent audio settings on both targets.

## Evidence

All 45 effects and 18 CDDA tracks are decoded. The mixer and actual Native/browser PCM gates cover music plus motor/countdown/impacts. Skid, crowd, commentary/menu effects, automatic selection and saved gains remain open. See src/audio/README.md and docs/rewrite.md.

## Next

Recover event/music selection rules and connect remaining sounds to shared simulation/menu events; retain rational resampling and device-lifetime checks.

## Accept

Reached original gameplay and menu audio events produce correct audible output on Native/browser; transport, selection, pause, volume and saved settings are exercised at the actual output boundary.
