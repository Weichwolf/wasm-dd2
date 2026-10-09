# Owned gameplay roads

`runtime/roads/` contains editable topology/geometry JSON, eleven `DD2ROAD1`
containers and their manifest. This is the user-requested intermediate baseline,
converted offline once; later authored tracks can replace these files. Normal
builds and runtime loading must not open the original archive or invoke its
converter. The already prepared visual meshes/textures are unchanged.

The current physics, grids, barriers, lap numbering and AI use signed fixed
positions at **160 units per meter**, with +Y up. Conversion to the owned
renderer is explicit division by 160. The container declares that convention;
it does not silently rescale physics tuning or claim original physical units.
Editable JSON uses `format: DD2ROAD1`, `layout: racing/arena`, `units_per_meter: 160`
and `up: Y`. Arrays describe positions, indexed strips and four-corner cells.
There are no original strip offsets, raw level records or relocation streams.

All fields are little-endian. No C struct padding or pointers are serialized.

| Record | Size | Fields |
| --- | ---: | --- |
| Header | 32 | Eight-byte `DD2ROAD1`, then u32 layout (0 racing/1 arena), units/meter, vertex count, strip count, main-loop count, cell count |
| Vertex | 12 | Three signed i32 positions in fixed units |
| Strip | 28 | Five u32: next, previous, branch, first cell, main order; u16 flags; u8 edge/junction kind, lane count, heading; three zero padding bytes |
| Cell | 28 | Four u32 vertex indices; u32 strip and lane; u8 surface flags, heading and triangle mask; one zero padding byte |

The byte extent must exactly match the header and contiguous arrays. Vertex and
strip counts are capped at 65,536; cell count at 1,000,000. Positions are limited
to +/-1,000,000 meters in the fixed convention. Racing strips own contiguous
lane ranges, with kind 1..11 and 1..127 lanes. Junction kinds 8/9 require a valid
branch; other kinds use `UINT32_MAX`. Next/previous indices must reach strip 0
without a disconnected cycle. Main orders describe exactly one forward loop
starting at strip 0; other strips use `UINT32_MAX`.

Cells reference bounded vertices and their exact lane owner. The quad order is
A, A+1, B+1, B; active triangle 0 uses corners 0,1,3 and triangle 1 uses 2,3,1.
Masks are 1..3. Arena roads have no strips/main loop and use `UINT32_MAX` cell
strip IDs. Their cells need not be a hardcoded 32x32 grid, although the converted
four arena baselines retain that geometry. Degenerate/projected-zero-area
triangles retain the existing contact behavior: no extrapolated contact.

`dd2_road_create_prepared` copies and validates the owned data. Source bytes may
be released immediately; getters borrow only until owner destruction. Legacy
source/provenance fields in the shared road type are zero for prepared roads.
The optional `dd2_road_create` reference decoder remains available for functional
comparisons, without becoming a prepared-file fallback.

`make assets-convert-roads` is the separate offline import command. It validates
the supported archive hash and translates independently decoded links to indices,
preserving existing cell order/tie behavior. `make assets-roads-prepare` recompiles
committed JSON without original input. `make assets-roads-verify` runs strict gates,
owned field/triangle-contact comparisons and the current starting-grid, barrier,
course and AI consumers on Native, Node/WASM and fresh O1 ASan/UBSan, including
malformed-file cleanup. Optional original-decoded consumer comparison is selected
explicitly with the verifier's `--reference-archive` argument.

These files remove the road factory's original-data dependency. The normal game
still needs its prepared provider/render, UI and audio migration; this component
does not prove original-executable parity, complete gameplay or standalone startup.
