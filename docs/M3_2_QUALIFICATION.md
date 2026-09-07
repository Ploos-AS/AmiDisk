# M3.2 qualification - disk against ADF verify

## Scope

M3.2 adds a read-only verification operation that compares a physical Amiga floppy in DF0-DF3 against a standard 880 KiB ADF image.

Command:

```text
AmiDisk verify-adf <unit> <path>
```

Neither the physical disk nor the ADF is modified.

## Implemented

- physical source validation through the qualified `trackdisk.device` backend
- exact standard ADF validation through the qualified M2 backend
- `TD_CHANGENUM` snapshot before and after the comparison
- complete 80 cylinder x 2 head x 11 sector comparison
- exact 512-byte sector comparisons
- count of compared sectors
- count of mismatching sectors
- first mismatch reported as cylinder/head/sector, byte within sector, absolute ADF byte offset, physical byte and ADF byte
- deterministic distinction between identical data, mismatch and I/O/status errors
- no-media handling before comparison
- controlled source-read and ADF-read failures

## Exit behavior

- return code 0: all 1760 sectors are identical
- return code 3: verification completed but at least one sector differs
- return code 2: source, media, ADF, I/O or media-change error

## Safety boundary

M3.2 is strictly read-only.

The verify engine contains no file-write, delete, rename, truncate, physical-write or format operation. `CMD_WRITE`, `TD_FORMAT`, `ETD_WRITE` and `ETD_FORMAT` remain forbidden before M3.3.

M3.1 remains the only write path and writes only to a newly-created ADF file.

## Required qualification

Before M3.2 is called complete:

1. `make clean && make check && make` passes with Bebbo GCC and `-m68000`.
2. `file AmiDisk` identifies an AmigaOS loadseg()-able executable.
3. Visible FS-UAE on Motorola 68000 / Kickstart 2.04 runs `AmiDisk --version` and reports `0.3.0-m3.2`.
4. Mount a known standard ADF as DF0 and make the same ADF accessible as a guest file.
5. `AmiDisk verify-adf 0 Work:reference.adf` returns 0 and reports 1760 identical sectors.
6. Create a second standard-size ADF with at least one known byte changed, without modifying the mounted source disk.
7. Verify against the modified ADF and require return code 3.
8. Confirm the reported first mismatch CHS, byte-in-sector, absolute offset and byte values match the intentional modification.
9. A wrong-size ADF is rejected through the M2 size gate.
10. A missing ADF is rejected in a controlled way.
11. With DF0 ejected, `verify-adf` reports no media and returns nonzero without modifying the ADF.
12. A real source-media change is observed with the already-qualified `qualify-media-change` path; source-change guarding in the verify engine is additionally inspected. If a deterministic swap during the verify loop can be performed, observe `AD_VERIFY_ERR_MEDIA_CHANGED` directly as well.
13. Existing M1 probe/read-sector regression tests pass.
14. Existing M2 ADF regression tests pass.
15. Existing M3.1 imaging regression passes.
16. Source audit confirms M3.2 introduces no write/destructive operation and no physical write/format command exists.
17. No crash, guru or hang is observed.

## Status

**IMPLEMENTED - native/runtime qualification pending.**
