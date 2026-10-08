Type: Work item
Title: Durable Native/browser save-card owner

## Contract

Own one complete original card and publish staged put/delete candidates only
after platform storage confirms success. Native uses an exclusive cooperating
writer lease, full-file synchronization, atomic replacement and directory sync.
Browser uses real IndexedDB read/write transactions with full-image optimistic
comparison and strict durability. Handle pending lifetime, repeated reopen,
corrupt/truncated storage, failed writes, process interruption and stale owners.
An ambiguous post-replacement sync failure requires reload; it is never success.

## Evidence

Closed 0055 owns physical/logical entries and closed 0056 owns typed profiles.
The main-thread C owner now stages complete borrowed replacements, publishes only
confirmed completion and refuses close/destroy while pending. Ordinary failures
preserve accepted memory and persisted images. Conflicts, invalid current storage
and ambiguous post-rename synchronization require explicit validated reload.

Ninety-two production Native/fresh O1 ASan/UBSan cases compare independently
predicted complete images and retain reserved bytes. Coverage includes missing
storage without creating a card, compact logical/physical mapping, duplicate/
bounds/full replacement, lease conflicts, partial/EINTR/zero/error writes, file
sync/close/rename failures, directory-sync ambiguity/recovery, external changes,
corrupt/truncated/extra storage, symlinks and nonblocking FIFO rejection. Two real
Native writers stop at publication boundaries; a live lease prevents another
owner from removing their temporary files. After SIGKILL and confirmed process
exit, a new owner recovers a complete old/new image and removes only the reserved
interrupted candidate.

Twenty-five actual Chromium/WASM checks compare full accepted/database images:
strict transaction completion, stale-owner conflicts/reload, a real enqueued
transaction abort, an injected synchronous enqueue exception, full process
close/relaunch restoring persisted data, corrupt reload/reopen, incorrect extent/
type and full-card replacement. Pending inputs are copied before caller disposal.
Delayed fake-backend ownership tests run separately on Native/WASM/sanitized C;
they do not establish actual platform persistence. Browser exceptions are zero.

Strict LLVM19 checks 185 C/header files; an additional active-WASM-branch tidy
check passes. All 41 Native and 39 WASM CTests pass; Native Memcheck finds zero
errors/leaks. Original game data and pinned SoftGL are unchanged. Successful raw
output, private binaries and completed browser profiles are removed after source,
binary and per-case hashes. Reports:
/tmp/wasm-dd2/rewrite-save-store-0057-final/{verification,quality,cleanup}-report.json.

## Next

Connect actual settings/frontend save/load through this owner under 0008/0003.
Translate and validate playable session state before replacing application state;
replays need a separate codec and rewrite Redbook gain needs explicit storage.
Native callers provide an existing dedicated writable directory; browser callers
provide a dedicated database. Cooperating leases/full-image comparison do not
provide atomic CAS against arbitrary noncooperating file-system writers.
The adapter alone does not establish saved playable championships.

## Accept

The previous memory image remains available until confirmed completion; ordinary
failed writes preserve disk and memory. Successful reopen restores all bytes.
Conflicts/indeterminate publication cannot silently retry or claim success.
Pending owners cannot be destroyed; completed operations release resources.
Strict LLVM19, Native/WASM gates, sanitized ownership and Memcheck pass. This
closes only durable card storage; complete settings/gameplay/replay/frontend
acceptance remains open.
