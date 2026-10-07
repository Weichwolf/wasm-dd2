Type: Work item
Title: English repository text and browser layout
Depends: 0014

## Contract

Remove the obsolete repository-level web/ directory and its generated legacy
browser output. Keep the active rewrite browser sources under src/platform/web/
and build output under /tmp/wasm-dd2/. Use English for repository documentation,
comments, commit messages and UI text, and record this convention in AGENTS.md.

## Evidence

The legacy shell and 614 MiB of ignored generated output were removed after
checking that no process used them. Its frozen Ghidra sources remain hash-checked
under /tmp. The legacy build entry point delegates to those frozen sources and
rejects repository output; original movies are provisioned as immutable links.
The legacy runtime was not rebuilt for this change.

README remains a concise English project description. The documented full goal,
Native title/messages and browser labels, status/error messages, controls and
accessibility text are English. The Native window verifier follows the new title.
Actual Native SDL startup and real Chromium checks pass: original archive load,
invalid-file status, countdown/pause/results, free driving, invalid music input,
Escape close and reopen. No browser page errors occurred.

Required gates pass: make rewrite-check (31 Native CTests plus clang-format 19
and strict clang-tidy 19 across 141 owned C/header files), make rewrite-wasm and
ctest --preset rewrite-wasm (30 CTests). Source/binary/log identities and scoped
receipts are in /tmp/wasm-dd2/rewrite-english-layout/report.json.

## Next

Keep repository text in English and generated output outside the worktree.
Continue the full game goal and natural contact diagnosis in 0002. These layout
and language checks do not prove complete gameplay or original parity.

## Accept

Root web/ is absent, the rewrite builds and its actual Native/browser UI works
with English text, frozen reference availability is preserved, and strict gates
pass. Commit and push the verified result.
