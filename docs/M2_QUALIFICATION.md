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

**M2 RUNTIME QUALIFICATION PASS** for the visible FS-UAE environment below.


## Runtime qualification — 2026-09-07

Starting HEAD and origin/main: `d9a32a1ca4e6558e8465e9a6e25a0bd027f86c1b`.
`git fetch origin`, checkout of `main`, and `git pull --ff-only origin main`
succeeded; starting worktree was clean. No AmiDisk code defect was found and
no source change was needed. The documentation commit containing this report
is the final qualification commit (its own hash is not embedded here).

### Emulator and assets

Visible, windowed FS-UAE 3.2.35, A500 with Motorola 68000; no headless run.
The emulator log confirms `CPU=68000, FPU=0, MMU=0, JIT=0`.
Guest `Version` reports `Kickstart version 37.175. Workbench version 37.67`
(Kickstart/Workbench 2.04), observed in [valid.png](evidence/m2/valid.png).
The exact launch command was:

```sh
fs-uae /tmp/amidisk-m2/qualification.fs-uae
```

Exact [configuration](evidence/m2/qualification.fs-uae):

```ini
[fs-uae]
amiga_model = A500
kickstart_file = /home/pgo/Documents/FS-UAE/Kickstarts/amiga-os-204.rom
chip_memory = 1024
slow_memory = 0
fast_memory = 4096
hard_drive_0 = /tmp/amidisk-m2/system
hard_drive_0_label = System
floppy_drive_count = 1
floppy_drive_0 = /tmp/amidisk-m2/system/test.adf
fullscreen = 0
window_width = 960
window_height = 720
base_dir = /tmp/amidisk-m2/fs-uae
hard_drive_0_priority = 10
uae_floppy_write_protect = true

```

The temporary `System:` host volume boots at priority 10. `Work:` is assigned
to `SYS:`; `C:` is assigned to `DF0:C` and `LIBS:` to `DF0:Libs` before testing.
The initial Assign executable was copied from the previous local M1 setup.
The [startup sequence](evidence/m2/startup-sequence) and four scripts
([valid](evidence/m2/valid), [sizes](evidence/m2/sizes),
[ranges](evidence/m2/ranges), [m1](evidence/m2/m1)) record the commands.
Each was run with `execute <script>` in the visible Amiga console, with
`FailAt 21` allowing expected error returns to continue. Each completed and
returned to the Shell prompt. [Log excerpt](evidence/m2/runtime-environment.txt)
records the actual emulator CPU and write-protection settings.

`Work:test.adf` is a copy of the locally supplied
`amiga-os-204-workbench.adf`, exactly **901120 bytes**, also mounted as DF0.
SHA-256: `9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef`.
The short fixture contains its first 901119 bytes; the long fixture contains
all 901120 bytes followed by one zero byte. Fixture preparation happened on
the host; no creation or mutation API was added to AmiDisk.

| Fixture | Bytes | SHA-256 |
| --- | ---: | --- |
| short.adf | 901119 | `9b0af5ae9b976646eb1135ceeb22ba9bdca7ceb96ee2e7574d3adafdea0d64b8` |
| long.adf | 901121 | `8b577061749dfdddae43e464dff57fa21b10e12995b9995da37f5e27d83fbe54` |

All three hashes were unchanged after runtime testing. ROM, Workbench image,
and copied OS executables are local user assets and are not committed.

### Observed results

Commands below use `Work:AmiDisk` to select the freshly built executable.

| Command arguments | Result | Observed output |
| --- | --- | --- |
| `--version` | PASS | `AmiDisk 0.2.0-m2`; `Motorola 68000 / AmigaOS 2.04+` |
| `adf-info Work:test.adf` | PASS | `ADF: Work:test.adf`; `size=901120 bytes cylinders=80 heads=2 sectors/track=11 sector-size=512` |
| `adf-read-sector Work:test.adf 0 0 0` | PASS | `ADF Work:test.adf C0 H0 S0 read OK`; bytes below |
| `adf-read-sector Work:test.adf 10 1 5` | PASS | `ADF Work:test.adf C10 H1 S5 read OK`; bytes below |
| `adf-info Work:short.adf` | PASS | `Work:short.adf: not a standard 880 KiB ADF` |
| `adf-info Work:long.adf` | PASS | `Work:long.adf: not a standard 880 KiB ADF` |
| `adf-info Work:does-not-exist.adf` | PASS | `Work:does-not-exist.adf: cannot open ADF` |
| `adf-read-sector Work:does-not-exist.adf 0 0 0` | PASS | `Work:does-not-exist.adf: cannot open ADF` |
| `adf-read-sector Work:test.adf 80 0 0` | PASS | `ADF Work:test.adf C80 H0 S0: sector address out of range` |
| `adf-read-sector Work:test.adf 0 2 0` | PASS | `ADF Work:test.adf C0 H2 S0: sector address out of range` |
| `adf-read-sector Work:test.adf 0 0 11` | PASS | `ADF Work:test.adf C0 H0 S11: sector address out of range` |
| `probe 0` | PASS | `DF0: media=present write-protected=yes` |
| `read-sector 0 0 0 0` | PASS | `DF0 C0 H0 S0 read OK`; bytes match ADF backend |

Evidence: [valid reads/version](evidence/m2/valid.png),
[size and missing-file errors](evidence/m2/sizes.png),
[range errors](evidence/m2/ranges.png),
[M1 regression and cross-check](evidence/m2/m1.png).

### Byte comparison

Host bytes were read directly with Python `Path.read_bytes()` and sliced at
`[offset:offset+16]`; displayed values below were independently observed in
the guest screenshots.

Sector C=0 H=0 S=0, offset 0 — host, ADF backend and DF0 all match:

```text
44 4f 53 00 e3 3d 0e 73 00 00 03 70 43 fa 00 3e
```

Sector C=10 H=1 S=5:
`(((10 * 2) + 1) * 11 + 5) * 512 = 120832`.
Host and ADF backend both show:

```text
00 00 00 08 00 00 00 e5 00 00 00 07 00 00 01 e8
```

This verifies the requested CHS mapping outside track zero against the raw
image at the exact calculated offset.

### Safety audit and control flow

Reviewed all application C sources and public backend headers, including
CLI dispatch and every trackdisk command call site. No active `CMD_WRITE`,
`TD_FORMAT`, `ETD_WRITE`, `ETD_FORMAT`, `fopen(..., "wb")`,
`fopen(..., "w")`, `fwrite`, image truncate, file delete/unlink,
rename-based overwrite, or ADF create/restore API exists. The only file open
is `fopen(path, "rb")`; subsequent file operations are size/position queries,
reads and close. Trackdisk requests remain limited to `TD_CHANGESTATE`,
`TD_PROTSTATUS` and `CMD_READ`. Exec `DeleteIORequest`/`DeleteMsgPort` release
resources and do not delete files. The M1 source is unchanged.

In `ad_adf_read_sector`, `ad_adf_chs_valid` rejects C >= 80, H >= 2 or S >= 11
and returns `AD_ADF_ERR_RANGE` before offset calculation, sector `fseek`,
and the sole `fread`. CLI open/size validation precedes this check and may
perform `fseek`/`ftell`, but does not call `fread`. Thus all three observed
range errors follow a control flow with no `fread`. This was verified by
source inspection, not runtime instrumentation of libc calls.

### Final host qualification and limits

Both initial and post-runtime `make clean`, `make check`, `make` and
`file AmiDisk` succeeded:

- M0 repository checks: PASS.
- M1 static checks: PASS.
- M2 static checks: PASS.
- Native Bebbo build: PASS, `/opt/amiga/bin/m68k-amigaos-gcc`,
  GCC `6.5.0b 20260807212032`.
- Compile flags: `-Isrc -Os -Wall -Wextra -Werror -m68000 -noixemul`.
- Link flags: `-noixemul`.
- `file AmiDisk`: `AmiDisk: AmigaOS loadseg()ble executable/binary`.
- Rebuilt executable and guest-tested copy have identical SHA-256:
  `dba6fe0007fd89aeec1c8bcdbf3fbf1456f79db8c8c14cd43b5c17ea526daf5d`.

Before documentation changes, HEAD and origin/main still matched the starting
commit, divergence was `0 0`, and the worktree was clean. Only this report
and M2 evidence are included in the qualification commit.

No AmiDisk crash, guru or hang was observed in any completed test. Initial
keyboard-layout errors in entering the script path were corrected by running
scripts from the current directory. An initial guest `Version` invocation
could not open version.library until `LIBS:` was assigned to DF0:Libs;
the corrected invocation reports both versions in valid.png. Neither setup
issue was counted as a passing test or an AmiDisk defect.

Qualification covers this visible 68000 / Kickstart 2.04 environment and
standard DD ADF reads. It does not qualify physical hardware, every OS release,
or injected seek/short-read faults. No write/destructive test was performed.
All mandatory M2 runtime cases above were observed; none is inferred from
static checks alone.

**M2 RUNTIME QUALIFICATION PASS**.
