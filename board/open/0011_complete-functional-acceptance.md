Type: Work item
Title: Complete playable Native and WASM acceptance
Depends: 0002, 0003, 0004, 0005, 0006, 0007, 0008, 0009, 0010, 0016, 0062

## Contract

Finish the full goal: every track, car, physics/damage/AI behavior, mode, menu, race/championship, replay, setting, keyboard/gamepad control, save/load and audio function works correctly on both targets; continue improving graphics.
The expanded product is standalone with all rebuilt assets committed in the
repository, including Blender cockpits, procedural textures and new audio.
Original DD2 files are never needed to build, launch or play. Actual graphics,
audio and player experience must substantially exceed the original in every area.

## Evidence

docs/rewrite.md inventories implemented components and their bounded evidence. The current viewer/practice application is not a complete game. Component checks cannot close this item.

## Next

Maintain a complete functional-surface inventory and close its gaps through the preceding work items. Reproduce all known issues and perform complete playable Native/browser acceptance when implementation is ready.

## Accept

Requirement-by-requirement current evidence proves the complete requested game and compatibility, strict LLVM19 gates pass, no known functional defects remain, and all completed improvements are committed/pushed. Do not redefine acceptance as a subset of current tests.
Both target distributions pass complete play/launch without original files or
provisioning. Editable asset sources, generators and runtime exports are versioned,
all content is replacement content, and visual/acoustic review proves the higher
quality with measured performance. Original-dependent tests cannot substitute
for standalone product acceptance.
