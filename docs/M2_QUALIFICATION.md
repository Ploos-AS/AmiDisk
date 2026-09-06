# M2 qualification - read-only standard ADF backend

## Scope

M2 introduces the first image backend. It accepts only ordinary 880 KiB Amiga DD ADF images and exposes read-only geometry and sector reads. M2 does not create, modify, restore or repair images and does not write physical media.

Reference geometry:

- 80 cylinders
- 2 heads
- 11 sectors per track per head
- 512 bytes per sector
- 1760 sectors
- 901120 bytes (880 KiB)

## Implemented

- binary read-only ADF open
- exact standard-size validation
- deterministic open/size/seek/read/range errors
- CHS-to-linear-sector mapping
- exact 512-byte sector reads
- explicit short-read failure
- `adf-info <path>` CLI
- `adf-read-sector <path> <cylinder> <head> <sector>` CLI
- first 16 bytes printed for diagnostic comparison
- static guard against ADF write/destructive APIs

## Safety boundary

M2 contains no ADF creation, write, truncate, rename or delete operation. The existing physical trackdisk backend remains read-only. `CMD_WRITE`, `TD_FORMAT`, `ETD_WRITE` and `ETD_FORMAT` remain outside the qualified scope.

## Required qualification

Before M2 is called complete:

1. `make clean && make check && make` passes with Bebbo GCC and `-m68000`.
2. `file AmiDisk` identifies an AmigaOS loadseg()-able executable.
3. Visible FS-UAE on the qualified 68000 / Kickstart 2.04 baseline runs `AmiDisk --version` and reports `0.2.0-m2`.
4. `AmiDisk adf-info known.adf` accepts a known-good 901120-byte ADF and reports the expected geometry.
5. `AmiDisk adf-read-sector known.adf 0 0 0` succeeds and its first 16 bytes match the source ADF.
6. A second sector away from track zero is read and compared against the source ADF, proving CHS mapping beyond offset zero.
7. A file shorter than 901120 bytes is rejected as wrong size.
8. A file longer than 901120 bytes is rejected as wrong size.
9. Missing/unreadable ADF is handled without crash or hang.
10. CHS values C=80, H=2 and S=11 are independently rejected before `fread`.
11. Existing M1 DF0 read/probe smoke tests still pass.
12. Source audit confirms no image or physical-media write/format path was introduced.

Record the exact emulator configuration and observed results here when runtime qualification is performed.

## Status

IMPLEMENTED - native/runtime qualification pending.
