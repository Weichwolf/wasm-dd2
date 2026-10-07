# Single-player league rules

`league.c` owns twenty stable driver IDs in four divisions of five. Division zero
is strongest; rank zero leads a division. The constructor preserves the original
initial field, placing human ID zero last in division three. Scores fit the
original nonnegative signed 16-bit fields. Every successful mutation preserves a
complete, unique division/rank permutation. Invalid scores, overflow and malformed
standings leave state and output unchanged.

A race adds up to 999 points per stable driver and reranks within each division;
ties favor the lower stable driver ID. Grid selection maps those IDs to source
physical slots: the overall leader occupies slot 19, the bottom driver slot zero.
`dd2_driving_create_grid` consumes this permutation and copies the assigned
starts, preserving human driver zero through reset and mode changes. Caller
storage is not retained; duplicate or out-of-range slots are rejected before
allocation. The default driving constructor uses the identity grid.

At season end, the top driver is promoted and the bottom driver relegated across
each adjacent boundary. All three swaps use the same final standings snapshot.
Transfers retain points; starting a new season clears them and reranks the new
divisions. Player first in the highest division wins the championship; last in
the lowest division is eliminated. Those terminal outcomes retain final scores.
The owning championship controller must handle rounds, history, unlocks, menus,
actual race results and saving separately; this module does not implement them.

`make rewrite-league-verify` compares 6,240 explicit original-x86 transfer, clear,
sort and end-of-season cases plus the original initial league on Native, WASM and
ASan/UBSan. The compact original fixture calls unmodified machine code and emits
only score/division/rank and classification, under one MiB. It checks component
rules, not full original parity or complete championships. The reference runner
requires GCC with 32-bit support (`gcc-multilib` on Debian) and provisioned dd2h.exe.

`make rewrite-grid-verify` exercises physical-grid ownership on all eleven
original levels and four fixture league divisions: 880 assigned starts per
target, settled twenty-car fields, short real Stockcar/Total Destruction drives,
withdrawal, reset and all seven circuits' one-car Time Trial transitions.
Fixture promotions set up standings; they are not actual championship results.
