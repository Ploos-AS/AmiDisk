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

## Runtime qualification — 2026-09-07

Qualification completed against the production binary, without injected errors
or instrumentation. Results below distinguish actual device observations from
source inspection.

### Environment and revisions

- Starting HEAD: `39eb19a085a1a1026f1c99e8fc845005fd1fa75d` on `main`, equal
  to `origin/main`, divergence `0 0`, clean worktree.
- Runtime-tested revision after CLI correction:
  `559df503e85ce6eab0977d3cfa3b6dd4b83e51c1`.
- Visible FS-UAE 3.2.35, A500, Motorola 68000, no FPU/MMU/JIT.
- Kickstart 2.04 / 37.175; Workbench 2.04 / 37.67, observed using guest `Version`.
- Disposable directory-backed Workbench system; `Work:` assigned to that system.
  Commands and ADF reference files remain accessible with DF0 ejected.
- DF0 uses `/tmp/amidisk-m32/original.adf`, with emulator write protection.
  No physical floppy device is attached or written.
- Native Bebbo `m68k-amigaos-gcc (GCC) 6.5.0b 20260807212032`.
  All compilation commands use `-m68000 -noixemul`.
- Initial clean/check/build gate: M0, M1, M2, M3.1, M3.2 static PASS;
  native build PASS. `file AmiDisk`:
  `AmiDisk: AmigaOS loadseg()ble executable/binary`.

### Controlled fixtures

DF0 and `Work:reference.adf` are byte-for-byte copies of the same known-good
standard Workbench ADF used for M3.1a qualification. Both contain 901120 bytes,
SHA-256 `9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef`.

The mismatch fixture changes exactly one byte: C10/H1/S5, byte-in-sector 7,
absolute offset `(((10 * 2) + 1) * 11 + 5) * 512 + 7 = 120839`.
Original/disk byte is `e5` (229), changed/ADF byte is `e4` (228), using XOR 1.
The mismatch file remains exactly 901120 bytes. The short fixture removes only
the last reference byte (901119 bytes); the long fixture appends one zero byte
(901121 bytes). The original is untouched. See [fixture hashes](evidence/m3.2/fixtures.json)
and the [reproducible fixture generator](evidence/m3.2/prepare-fixtures.py).

Fixture creation and the separately requested M3.1 smoke test are the only
intentional new ADF writes. `verify-adf` never writes an ADF or the mounted disk.

### CLI correction and retest

The first runtime pass correctly reported one mismatching sector, C10/H1/S5,
byte 7, offset 120839, disk `e5`, ADF `e4`, RC 3. Its CLI label was
`verify-adf mismatch`, omitting the requested literal `data mismatch`.
Commit `559df503e85ce6eab0977d3cfa3b6dd4b83e51c1` changes only that label to
`verify-adf data mismatch`. It was committed and pushed to `main`, then the
clean/check/build gate was repeated and the rebuilt binary copied to the guest.
The verify algorithm, return codes, backends and imaging path are unchanged.
The initial screenshot is retained as [pre-fix evidence](evidence/m3.2/normal-before-cli-fix.png).

### Observed runtime results on the rebuilt binary

[Normal results screenshot](evidence/m3.2/normal.png). Each script captures `$RC`
immediately after AmiDisk, before `Type` or any other command. Standard output
and return codes are saved as `m32-*.out` / `m32-*.rc`; diagnostics written to
stderr remain on the visible Amiga console and are preserved in screenshots.

| Test | Observed result | RC |
| --- | --- | --- |
| Version | `AmiDisk 0.3.0-m3.2`, `Motorola 68000 / AmigaOS 2.04+` | 0 |
| Initial DF0 probe | `media=present write-protected=yes` | 0 |
| Identical verify | `verify-adf OK: identical sectors=1760 change=0`; no mismatch | 0 |
| Controlled mismatch | `verify-adf data mismatch: sectors=1 first=C10 H1 S5 byte=7 offset=120839 disk=e5 adf=e4` | 3 |
| Short ADF, 901119 bytes | `cannot open reference ADF (not a standard 880 KiB ADF)` | 2 |
| Long ADF, 901121 bytes | `cannot open reference ADF (not a standard 880 KiB ADF)` | 2 |
| Missing ADF | `cannot open reference ADF (cannot open ADF)` | 2 |
| DF0 ejected probe | `media=absent write-protected=no` | 0 |
| No-media verify | `verify-adf failed: no media present` | 2 |

Every reported mismatch coordinate and byte value matches the host's one-byte
change exactly. Both the initial binary and rebuilt binary completed identical,
mismatch, short, long and missing-file tests. No-media ran on the rebuilt binary.

### Actual media-change observations

One successful invocation of `qualify-media-change 0` held the actual device
handle open while DF0 was ejected using the visible FS-UAE menu:

```text
DF0 before: media=present change=0
Eject or swap the disk in FS-UAE now, then press RETURN.
DF0 after: media=absent change=1
qualification OK: trackdisk media change observed
```

RC was 0. See [before](evidence/m3.2/change-before.png) and
[after](evidence/m3.2/change-after.png). Initial GUI navigation selected a disabled
drive entry; it did not perform a media action. After selecting DF0 and Eject,
the menu was briefly reopened and then closed before RETURN reached AmiDisk.
The diagnostic stayed pending throughout. Only one actual eject occurred in this
invocation. [No-media](evidence/m3.2/no-media.png) was then tested with DF0 empty.

The source was remounted through the GUI. A separate, uninstrumented production
`verify-adf 0 Work:reference.adf` was started after a successful present-media
probe. The [start screenshot](evidence/m3.2/verify-change-start.png) shows verify
running, before any completion. During the comparison, the FS-UAE DF0 Eject menu
was used again. This single mid-verify attempt produced:

```text
verify-adf failed: source read failed (trackdisk.device I/O error)
2
VERIFY CHANGE RETURNED
```

See [runtime failure and shell return](evidence/m3.2/verify-change-result.png).
This is a controlled production verify source-read failure caused by a real eject,
not a simulated error. No crash, guru or hang occurred.

- **Runtime trackdisk change detection: PASS** (`TD_CHANGENUM` 0 → 1).
- **Runtime eject during verify: PASS**, source-read failure, RC 2.
- **Verify-specific `AD_VERIFY_ERR_MEDIA_CHANGED` branch: statically verified;
  NOT runtime-observed.** The earlier read failure returns before the end check.
  Source inspection confirms the starting number is read before comparison,
  the ending number after all sectors, and unequal values return
  `AD_VERIFY_ERR_MEDIA_CHANGED` before either success or mismatch is returned.

A keyboard-layout mismatch affected two preliminary manually typed commands
(colon became `*`, hyphen became `/`). They returned ordinary command/usage
errors. The intended tests were then invoked using correctly encoded guest
scripts. These input mistakes are not test results or application crashes.

### Read-only safety audit

- Verify compares `ad_td_read_sector` and `ad_adf_read_sector` buffers with
  `memcmp`, and snapshots `TD_CHANGENUM` before and after the entire loop.
- The ADF backend opens only `fopen(path, "rb")`; seeks and reads never write.
- Verify introduces no `CMD_WRITE`, `TD_FORMAT`, `ETD_WRITE`, `ETD_FORMAT`,
  `fwrite`, write-mode `fopen`, truncate, delete, rename, overwrite or restore.
- Trackdisk commands are limited to `CMD_READ`, `TD_CHANGESTATE`,
  `TD_PROTSTATUS` and `TD_CHANGENUM`. Open/close and status queries support the
  read-only comparison; there is no physical-write path.
- `src/operations/image_adf.c`, `src/io/trackdisk/trackdisk.c` and
  `src/io/adf/adf.c` have no diff against qualified M3.1a revision
  `087900c3d0e0b7955397f66ca19142e199a46727`.
- M3.1 still creates only a new destination with `O_CREAT | O_EXCL`, and its
  existing-file protection and failed-output cleanup are unchanged.

### M1/M2/M3.1 regressions and integrity

After the second eject, `original.adf` was remounted through the visible GUI.
All regression commands returned RC 0. See [regression screenshot](evidence/m3.2/regression.png)
and [probe/start screenshot](evidence/m3.2/regression-progress.png).

| Command | Runtime result |
| --- | --- |
| `AmiDisk probe 0` | `media=present write-protected=yes` |
| `AmiDisk read-sector 0 0 0 0` | `DF0 C0 H0 S0 read OK` |
| `AmiDisk adf-info Work:reference.adf` | 901120 bytes, 80 cylinders, 2 heads, 11 sectors/track, 512-byte sectors |
| `AmiDisk adf-read-sector Work:reference.adf 0 0 0` | `C0 H0 S0 read OK`, same bytes as DF0 |
| `AmiDisk image-adf 0 Work:m32-regression.adf` | `image-adf OK: sectors=1760 bytes=901120 change=4` |
| `AmiDisk adf-info Work:m32-regression.adf` | Standard 901120-byte ADF accepted |

Both sector reads begin with:
`44 4f 53 00 e3 3d 0e 73 00 00 03 70 43 fa 00 3e`.
The imaging destination did not exist before this one invocation. No existing
destination was overwritten. Host byte-for-byte comparison of all 901120 output
bytes against the mounted reference passed; both SHA-256 values are
`9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef`.

[Final integrity evidence](evidence/m3.2/after-hashes.json) confirms DF0's backing
image, reference, mismatch, short and long fixtures all retain their original
size and SHA-256 after runtime, including both eject tests. The missing path
remains absent. No physical disk-write occurred. No verify ADF-write occurred.

### Final host gate and delivery

After runtime, `make clean`, `make check`, `make` and `file AmiDisk` all passed.
M0 static PASS, M1 static PASS, M2 static PASS, M3.1 static PASS,
M3.2 static PASS, native Bebbo build PASS. See [host gate](evidence/m3.2/host-verification.txt).
The rebuilt binary is byte-for-byte identical to the guest binary used for the
final runtime tests, SHA-256:
`ae6a7f140bce5e59b5d63f9a763d1ea9191027bae28bf8bac3dbd1657cd54feb`.

At this gate, source HEAD and `origin/main` both equal
`559df503e85ce6eab0977d3cfa3b6dd4b83e51c1`, divergence `0 0`; the only pending
changes are this report and its evidence. Final delivery HEAD is the report
commit containing this section, resolvable using
`git log -1 --format=%H -- docs/evidence/m3.2/host-verification.txt`.
Its literal hash and post-push repository state are reported in the delivery
response, avoiding a self-referential commit hash inside its own contents.

## Status

**M3.2 RUNTIME QUALIFICATION PASS.** Identical, exact controlled mismatch,
wrong-size and missing files, real no-media, actual trackdisk change detection,
actual eject during verify, M1/M2/M3.1 regressions and read-only audit all PASS.
No crash, guru or hang was observed; completed commands returned to the shell.
The diagnostic's intentional RETURN wait was not a hang.

The exact end-of-verify `AD_VERIFY_ERR_MEDIA_CHANGED` branch remains **statically
verified, not runtime-observed**. The actual eject during verify was detected
earlier through a controlled source-read failure, RC 2. This distinction is not
included as an unobserved runtime branch PASS.
