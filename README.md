# wasm-dd2 — Destruction Derby 2 (dd2h.exe) → WebAssembly

A faithful port of the 1996 game **Destruction Derby 2** (`dd2h.exe`, 640x480 build) to reproducible C
compiling to native and WebAssembly. The engine C is **mechanically derived from the binary via Ghidra**
(decompile → transpile/patch → compile); only the platform/runtime shim (DirectDraw→canvas,
DirectSound→WebAudio, Win32/CRT) is hand-written.

Pipeline: `dd2h.exe → tools/decompile.sh → tools/transpile.py → tools/build*.sh`.
See **CLAUDE.md** for build commands, current status, and conventions.

## Contributors

- **Weichwolf** — project owner, direction, and hands-on testing
  (keyboard + gamepad, native + browser).
- **Claude** (Anthropic) — decompile-to-C reconstruction, patch authoring, and build/verify tooling.

This was a collaborative effort — the reconstruction, testing, and direction were shared.
