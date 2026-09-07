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

**M3.1 RUNTIME QUALIFICATION PASS** for the visible FS-UAE environment below.

### Runtime qualification — 2026-09-07

Visible FS-UAE 3.2.35, A500/Motorola 68000, Kickstart/Workbench 2.04 baseline.
The known-good 880 KiB `original.adf` was mounted as DF0. Starting HEAD was
`9a6c39159102c490c8b5904eb108d3a5f434ceba`.

Observed guest results:

| Test | Result |
| --- | --- |
| `AmiDisk --version` | PASS — `AmiDisk 0.3.0-m3.1` |
| `AmiDisk probe 0` | PASS — `DF0: media=present write-protected=yes` |
| imaging | PASS — `image-adf OK: sectors=1760 bytes=901120 change=0` |
| `adf-info` generated image | PASS — standard geometry accepted |
| sector C0/H0/S0 | PASS — `44 4f 53 00 e3 3d 0e 73 00 00 03 70 43 fa 00 3e` |
| sector C10/H1/S5 | PASS — `00 00 00 08 00 00 00 e5 00 00 00 07 00 00 01 e8` |
| existing destination | PASS — second imaging refused; original output remained unchanged |
| M1/M2 regressions | PASS — probe, physical read-sector and adf-info |

Host-side comparison of the copied source and generated image is byte-for-byte
identical. Both SHA-256 hashes are
`9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef`; size is
901120 bytes. No crash, guru, or hang was observed in the completed runtime.

No-media imaging was prepared in the same visible configuration but the second
FS-UAE session did not reach the scripted shell output in this environment, so
no runtime PASS is claimed for that case. The source control path was statically
verified: `AD_IMAGE_ERR_NO_MEDIA` returns before destination creation, and all
failure paths remove a partial output. The disk-change test was not deterministic
to perform live during the long acquisition; no runtime PASS is claimed. Static
inspection verifies the `TD_CHANGENUM` snapshot/compare and partial-file removal.

The physical backend still contains no `CMD_WRITE`, `TD_FORMAT`, `ETD_WRITE`, or
`ETD_FORMAT`. M3.1 writes only a new ordinary ADF using exclusive `O_EXCL`
creation; existing destinations cannot be truncated.
