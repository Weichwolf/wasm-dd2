# Physics

Own vehicle motion, collision and damage with an explicit simulation step.
Keep rendering and operating-system calls out of this module. Verify stable
behavior on native and WASM, including fast-math edge cases.

`road_contact.c` provides vertical contact with one known road/arena lane cell.
It selects an active triangle in XZ, includes shared edges, rejects missing or
zero-area triangles and returns its interpolated height and unit normal with
positive Y. Calculations use double precision and convert source coordinates
before subtracting, avoiding signed integer overflow at coordinate extremes.
Queries must be finite. Edge tolerance is 1e-6 in world coordinates, converted
to each edge's barycentric scale; it cannot grow with the cell size. An expanded
triangle bounding rectangle also limits extrapolation at acute corners. Points
beyond this numerical tolerance fail. Signed-coordinate extremes and a point
one unit outside an extremely large cell are checked on both targets.

`road_surface.c` selects a contact without knowing its cell beforehand. Create
one surface index for a decoded road and destroy it before destroying that road:
the index owns its balanced XZ bounds hierarchy and borrows immutable geometry.
Construction and destruction allocate; queries have no allocations, rendering
or operating-system calls. Both construction and traversal are iterative.

A query supplies XZ coordinates, a finite inclusive height window and an optional
preferred cell (`DD2_ROAD_NO_STRIP` for none). It returns the highest eligible
contact. Contacts within 1e-6 below the global maximum form a tie: the preferred
cell wins if eligible, otherwise the lowest cell index wins. Two traversal
passes anchor ties to the global maximum, avoiding order-dependent tolerance
chains. Height windows distinguish stacked road surfaces; they do not model
wheel travel, motion through walls or recovery outside the track. Invalid
coordinates/windows fail and clear the result and optional statistics. An
integer byte-representation check rejects NaN and infinity even with the
required fast-math compiler flags.

Synthetic CTests run on native and WASM. `make rewrite-road-verify` checks the
original contact geometry against independent plane calculations with ASan/UBSan
as well. Source contact normals are not used, and original fixed-point height
rounding is not a rewrite requirement. `make rewrite-surface-verify` also compares
surface selection with an exhaustive, ID-ordered cell search on all eleven
original levels, on native, Node/WASM and ASan/UBSan. Each target checks 197,750
queries covering both triangle centroids with three height windows and all cell
corners. The oracle uses the separately verified contact primitive, without the
index hierarchy. Selection traces and work counts agree across targets. The
index performs about 4.1–4.8 actual cell contact tests per query on average,
including both passes, and the verifier requires at least 95% pruning relative
to a full cell scan. These counts describe the sampled queries, not a guarantee
for arbitrary geometry or a timing benchmark.

Synthetic tests include stacked planes, strict height boundaries, shared edges,
preferred-cell ties, tolerance chains, acute corners, coordinate extremes and
NaN/infinity under fast-math. The viewer does not yet use this physics module.
Wheel suspension, drivetrain, steering, jumps, collisions and damage remain to
be implemented; these checks establish contact selection, not driving parity.
