Type: Work item
Title: Complete sound effects and Redbook selection

## Contract

Provide all reached gameplay/menu/commentary effects, pitch/spatial behavior, automatic game-state Redbook selection and persistent audio settings on both targets.

## Evidence

All 45 effects and 18 CDDA tracks are decoded. The mixer and actual Native/browser PCM gates cover music plus motor/countdown/impacts. Skid, crowd, commentary/menu effects and automatic selection remain open. See src/audio/README.md and docs/rewrite.md.

Player/car/audio profiles now persist actual effects and music gains through
restart under 0058. Their output regression is recorded at
/tmp/wasm-dd2/rewrite-saved-car-0058-audio/verification-report.json.

Active 0060 now has a reproducible original selection contract. Forty-eight
unmodified-original transport/countdown checks, six caller contexts and the
independently decoded eleven-entry menu-to-asset table establish menu/practice
track 13, racing loaded-level + 1 at GO, championship result/standings track 14
and end-of-season track 15. CD start, explicit pause/resume, repeat guards,
disabled audio and failed start are checked with protected original code.
Receipt: /tmp/wasm-dd2/rewrite-redbook-policy-0009/verified-3/report.json.
The rewrite still selects music manually; automatic output remains unproved.

Actual effects/Redbook output regressions after championship application
integration pass on Native, ASan/UBSan and real WebAudio at 44,100/48,000 Hz:
/tmp/wasm-dd2/rewrite-championship-effects-ack-current/report.json and
/tmp/wasm-dd2/rewrite-championship-music-current/report.json. The Native effect
pause observer acknowledges a visible processed event before checking the PCM
sink; this avoids sampling an instrumented frame before Pause has executed.

## Next

Recover event/music selection rules and connect remaining sounds to shared simulation/menu events; retain rational resampling and device-lifetime checks.

## Accept

Reached original gameplay and menu audio events produce correct audible output on Native/browser; transport, selection, pause, volume and saved settings are exercised at the actual output boundary.
