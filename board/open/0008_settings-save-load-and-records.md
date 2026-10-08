Type: Work item
Title: Settings, save/load and original data compatibility

## Contract

Persist and restore profiles, names, championships, unlocks, settings and track records with original format compatibility and atomic failure handling.

## Evidence

Session Time Trial clocks work. Persistent records/settings and the full save owner are absent. Original SaveGames card layout and reference fixtures exist, including fifteen slots and source championship/statistics regions.

An unmodified-original 38-key run saves Configuration as A, stages and commits
Sound Effects from 4090 to 3681, and loads A to restore 4090. The first 0x197e
payload bytes match the original packed configuration, beginning with magic
0x1010; the card has fifteen 0x200-byte headers and 0x2000-byte payload slots
within 0x20000 bytes. These are observed configuration bytes, not a complete
typed codec. Occupied overwrite/delete cancellation and load preserve every
card byte; accepted deletion clears the occupied marker/name prefix and retains
the old payload. Nine X11 windows are reviewed and the provisioned save remains
unchanged. Receipt:
/tmp/wasm-dd2/rewrite-frontend-occupied-card-0003-5/report.json.
Game/championship/replay data, accepted overwrite, multiple occupied slots,
corrupt/full/failed storage and rewrite adapters remain unproved.

## Next

Document supported file layouts, implement typed codecs plus Native/browser storage adapters, and connect ownership to settings, championship and replay flows.

## Accept

Independent original-file reads and write/read round trips cover valid files, full slots, corrupt/truncated input, failed writes and repeated reloads on both targets without partial state mutation.
