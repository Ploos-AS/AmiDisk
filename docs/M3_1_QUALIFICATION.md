# M3.1 qualification - disk to standard ADF imaging

## Scope

M3.1 introduces AmiDisk's first controlled write path. It writes only to a newly-created standard ADF file. The physical `trackdisk.device` backend remains strictly read-only.

Command:

```text
AmiDisk image-adf <unit> <path>
```

The source is DF0-DF3 and the destination is a new ordinary 880 KiB ADF image.

## Implemented

- source-drive open and media-presence validation
- refusal to overwrite an existing destination path
- `TD_CHANGENUM` snapshot before and after acquisition
- 80 cylinder x 2 head x 11 sector acquisition using the qualified M1 sector reader
- exact 512-byte writes to the destination image
- exact 1760-sector / 901120-byte successful output
- partial output removal after source-read or destination-write failure
- output removal if the source media change number differs after acquisition
- deterministic imaging error strings and CLI status

## Safety boundary

M3.1 intentionally permits `fopen(..., "wb")`, `fwrite` and `remove` only for the newly-created destination ADF/failed partial output path.

M3.1 still contains no physical-media write or format operation. `CMD_WRITE`, `TD_FORMAT`, `ETD_WRITE` and `ETD_FORMAT` remain forbidden.

An existing destination file must never be overwritten or truncated.

## Required qualification

Before M3.1 is called complete:

1. `make clean && make check && make` passes with Bebbo GCC and `-m68000`.
2. `file AmiDisk` identifies an AmigaOS loadseg()-able executable.
3. Visible FS-UAE on Motorola 68000 / Kickstart 2.04 runs `AmiDisk --version` and reports `0.3.0-m3.1`.
4. Image a known-good DF0 disk to a previously nonexistent path using `AmiDisk image-adf 0 Work:imaged.adf`.
5. Successful output reports 1760 sectors and 901120 bytes.
6. `AmiDisk adf-info Work:imaged.adf` accepts the produced image.
7. At least sector C0/H0/S0 and one non-zero-track sector from the produced ADF match the source disk bytes.
8. Host-side byte-for-byte comparison against the original source ADF is performed when the FS-UAE source disk itself is backed by a known ADF; the produced image must be identical.
9. Re-running `image-adf` with the same destination path is rejected and the existing file remains byte-for-byte unchanged.
10. Imaging with no media is rejected and leaves no output file.
11. Invalid/unavailable DF1-DF3 source handling is controlled and leaves no output file.
12. A source-media change during acquisition is detected by the change-number guard and the partial output is removed. If emulator timing makes this test impractical, document the exact attempted procedure and independently prove the guard path by code inspection; do not claim the runtime subtest was observed.
13. Existing M1 probe/read-sector regression tests pass.
14. Existing M2 adf-info/adf-read-sector regression tests pass.
15. Source audit confirms no physical-media write/format command was introduced.
16. No crash, guru or hang is observed.

## Status

IMPLEMENTED - native/runtime qualification pending.
