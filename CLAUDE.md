# Development entry points

Read [AGENTS.md](AGENTS.md), [board/README.md](board/README.md) and the current work
item. Architecture, supported features and required gates are in
[docs/rewrite.md](docs/rewrite.md).

`master` contains the handwritten C11 rewrite. `ghidra` and the immutable
`reconstruction-baseline` tag preserve the decompiled/patched reference.
`make reference-prepare` supplies hash-checked frozen reference sources under
`/tmp/wasm-dd2/`; `make reference REFERENCE_TARGET=check` runs its commands there.
Original game data and reusable dependencies remain ignored. SoftGL is our pinned
`deps/softgl` submodule; other local dependencies also live under `deps/`.
