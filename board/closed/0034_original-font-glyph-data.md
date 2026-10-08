Type: Work item
Title: Decode original font glyph data
Depends: 0001

## Contract

Provide owned, validated original FONT.BNK glyph tables for shared C text
rendering. Preserve all three 96-character tables and reject malformed extents
without reading beyond the logical asset on Native and WASM.

## Evidence

The handwritten C11 font bank copies character UV/width/height into owned
storage. Character 32..127 lookup, unavailable zero-sized glyphs, output clearing,
source destruction and null cleanup are checked. The shipped final record
advertises an unused trailing word beyond the logical file; glyph reads stay
inside the declared bytes. Explicit final padding produces the same tables.

Native, Node/WASM and ASan/UBSan compare every byte of all 288 original glyphs
with the independent file reader and reject twelve mutated original banks.
A separate read-only observation of ordinary original Wine frontend startup
checks all 576 loaded glyphs across the three base fonts and three duplicates.
Both save copies remain unchanged; no debugger or engine write is used.
Receipt: /tmp/wasm-dd2/rewrite-font-verification-complete/report.json.

Mandatory format/strict LLVM 19 analysis and 35 Native / 33 WASM CTests pass.
Quality receipt: /tmp/wasm-dd2/rewrite-font-verification-complete/quality-report.json.
Successful raw bank copies, exports, sanitizer binary and Wine logs are removed.

## Next

Decode sprite/font texture bindings and connect actual shared text/menu rendering
under 0003. Glyph data alone does not establish menu rendering, navigation,
editable settings, persistence or a complete game.

## Accept

All original glyph bytes match on three targets and the live original; input
lifetime and malformed extent/character checks pass. Only the glyph-data
contract is closed.
