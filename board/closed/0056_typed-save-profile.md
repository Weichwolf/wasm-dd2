Type: Work item
Title: Typed original configuration and profile payload

## Contract

Decode and encode the complete original 8192-byte configuration/profile block
into owned C11 fields: settings, five season records, current drivers, player
names, seven five-entry lap tables and controller bindings. Preserve reserved
bytes and validate structural input transactionally. Original replay blocks use
a different schema. Parsing is separate from playable-session validation.

## Evidence

Closed 0055 supplies the selected physical block. An additional actual-original
30-key run saves A/B with distinct effects levels and checks packed bytes against
five bounded independent engine regions. Three actual X11 windows are reviewed.
The original's field at payload byte 18 is controller type, not CD volume; its
input poll sets keyboard type 1 and joystick type 2. The configuration packer does
not include a separate Redbook gain. Receipt:
/tmp/wasm-dd2/rewrite-original-profile-0056/report.json.

The value codec now owns every defined field and retains driver/suffix data.
Original destruction counters and current/final race places are named separately
from scores. Three hundred independent comparisons cover 48 patterned immutable
original-x86 packs (all three supported kinds) and two physically observed A/B
configurations on Native, WASM and fresh O1 ASan/UBSan. Each checks 1021 typed
values and all 8192 encoded bytes: 306300 values total, including independently
predicted edits across every region. Source/executable/binary identities remain
stable. Every truncated extent, six unsupported kinds, all 320 malformed fixed
text fields, encoder rollback and overlapping storage pass focused checks.
Native Memcheck proves zero errors/leaks; strict LLVM19 checks 178 C/header
files, with 40 Native and 38 WASM CTests passing. Reports:
/tmp/wasm-dd2/rewrite-save-profile-0056/{verification,quality,cleanup}-report.json.
Successful raw output, private verification binaries and completed original
captures are removed after receipts. Original game data remains unchanged.

## Next

Translate and validate complete source configuration/game state, add durable
Native/browser publication and connect real frontend actions under 0008/0003.
Retain physical slot identity through import; stage failed writes without
publishing partial application state. Replays use a separate codec.
Numeric parsing alone never accepts a playable session; the codec does not
establish restart, settings application or championship continuation.

## Accept

All original fields and retained bytes agree; independently predicted edits
change only their expected offsets. Truncation, unsupported magic and malformed
fixed text preserve caller state/output; overlapping encode/decode is staged.
Strict LLVM19, Native/WASM CTests, ASan/UBSan and Memcheck pass. This closes only
the codec, not durable writes or playable saved championships/replays.
