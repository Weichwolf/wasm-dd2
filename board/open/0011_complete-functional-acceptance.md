Type: Work item
Title: Complete playable Native and WASM acceptance
Depends: 0002, 0003, 0004, 0005, 0006, 0007, 0008, 0009, 0010, 0016

## Contract

Finish the full goal: every track, car, physics/damage/AI behavior, mode, menu, race/championship, replay, setting, keyboard/gamepad control, save/load and audio function works correctly on both targets; continue improving graphics.

## Evidence

docs/rewrite.md inventories implemented components and their bounded evidence. The current viewer/practice application is not a complete game. Component checks cannot close this item.

## Next

Maintain a complete functional-surface inventory and close its gaps through the preceding work items. Reproduce all known issues and perform complete playable Native/browser acceptance when implementation is ready.

## Accept

Requirement-by-requirement current evidence proves the complete requested game and compatibility, strict LLVM19 gates pass, no known functional defects remain, and all completed improvements are committed/pushed. Do not redefine acceptance as a subset of current tests.
