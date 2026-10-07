Type: Work item
Title: Complete input and platform lifecycle

## Contract

Implement keyboard and gamepad controls, bindings, device changes, focus/pause and Native/browser lifecycle for every game/menu flow.

## Evidence

Keyboard, focus suspension and window/canvas lifecycle exist. Discrete SDL
commands now execute before later queued control transitions, preserving restart,
pause, withdrawal and throttle order. A focused Native test covers queued order,
pressure, release, repeat and wheel input. Actual Native, ASan/UBSan and Chromium
regressions pass for all eleven views and the existing practice/championship modes;
owned browser-loop close/reopen also passes. Evidence:
/tmp/wasm-dd2/rewrite-championship-application-window-current/report.json.
Complete gamepad support, rebinding and saved device settings remain pending.

## Next

Extend typed input actions to complete menus and gamepad/device ownership; exercise real input through menus, races and settings with disconnect/reconnect.

## Accept

Both targets honor bindings and controller input, release stale buttons on focus/device changes, and survive repeated open/close/reset cycles.
