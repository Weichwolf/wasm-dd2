# 0055: Own the original Windows save-card container

## Contract

Decode and own the exact 128 KiB Windows SaveGames container with fifteen physical
headers/payloads. Expose compact logical entries, preserve complete opaque blocks
and reserved bytes, and stage aliased replacement inputs before mutation. Every
invalid/full/duplicate operation preserves the entire image. This is a container
contract; typed settings/game/replay codecs, durable adapters and frontend actions
remain under 0008/0003.

## Evidence

The actual original 63-key multiple-configuration run proves A/B physical slots
0/1, B's logical compaction to zero after deleting A, loading B without changing
any bytes, and C's first-free allocation back to physical zero. Ten actual X11
windows and six bounded card checkpoints are reviewed; provisioned data remains
unchanged. Receipt:
/tmp/wasm-dd2/rewrite-frontend-multiple-card-0008-3/report.json.

A private handwritten C11 candidate passes strict LLVM19 analysis and fresh
Native/WASM/O1 ASan/UBSan synthetic checks. Eighteen original complete image reads
and twelve complete original put/delete mutations match every byte across the
three targets. Synthetic cases cover every truncated extent, every malformed
physical header, full append, duplicate rollback, maximum/empty names, borrowed
header-overlapping input, compaction/replacement and owned reopen lifetime.
Receipt: /tmp/wasm-dd2/rewrite-save-card-0055/private-comparison-report.json.

## Verified production result

The original 68-key follow-up reproduces duplicate B/B creation after compaction
and loading the first B's 3272 effects volume when the second B's 3681 payload
was selected. Ten actual windows and seven card checkpoints are reviewed. New
uniqueness checks use physical identity and reject that insertion without any
mutation; existing duplicate-name images remain readable as distinct physical
entries. Receipt: /tmp/wasm-dd2/rewrite-frontend-duplicate-card-0008-1/report.json.

Production Native/WASM/fresh O1 ASan/UBSan pass 66 independent comparisons:
24 exact image reads/inventories, 27 complete selected 8 KiB physical payloads,
12 exact actual-original mutations and three compacted duplicate rejections.
Synthetic checks additionally preserve unrelated headers/name tails/unused
blocks filled with nonzero data, plus bounds/ownership/rollback cases above.
Strict LLVM19 checks all 174 C/header files; 39 Native and 37 WASM CTests pass.
Native Memcheck reports zero errors and no allocated blocks at exit. Mandatory
flags and the pinned SoftGL dependency are unchanged. Completed original images,
raw successful output and private binaries are removed after recording receipts.
Receipt: /tmp/wasm-dd2/rewrite-save-card-0055/verification-report.json.

## Next

The opaque container contract is proved. Continue typed settings/game/replay
payloads and staged durable Native/browser publication under 0008, then connect
the actual frontend actions under 0003. No in-memory mutation is a durable save.
The separate Arena-8 observer retains its frozen linked binary and original
controls; this standalone asset module does not alter its owner in progress.

## Accept

Production Native/WASM/fresh sanitizer ownership/bounds/rollback checks pass;
independent original image/entry reads and actual full-image mutation comparisons
pass. Strict LLVM19 and required build/CTest gates pass. Durable save success and
full settings/game/replay/frontend compatibility are explicitly outside this item.
