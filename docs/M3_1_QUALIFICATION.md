# M3.1 qualification - disk to standard ADF imaging

## Scope

M3.1 introduces AmiDisk's first controlled write path. It writes only to a newly-created standard ADF file. The physical `trackdisk.device` backend remains strictly read-only.

Command:

```text
AmiDisk image-adf <unit> <path>
```

The source is DF0-DF3 and the destination is a new ordinary 880 KiB ADF image.

M3.1a adds an interactive qualification-only diagnostic command:

```text
AmiDisk qualify-media-change <unit>
```

This command does not write media or image files. It keeps one `trackdisk.device`
handle open, records `TD_CHANGESTATE`/`TD_CHANGENUM`, waits for the operator to
eject or swap media in visible FS-UAE, then observes the real device state again.

## Implemented

- source-drive open and media-presence validation
- refusal to overwrite an existing destination path
- exclusive destination creation using `O_EXCL`
- `TD_CHANGENUM` snapshot before and after acquisition
- 80 cylinder x 2 head x 11 sector acquisition using the qualified M1 sector reader
- exact 512-byte writes to the destination image
- exact 1760-sector / 901120-byte successful output
- partial output removal after source-read or destination-write failure
- output removal if the source media change number differs after acquisition
- deterministic imaging error strings and CLI status
- M3.1a interactive observation of actual `trackdisk.device` media-change state

## Safety boundary

M3.1 permits `fwrite` and removal only for the newly-created destination ADF or a failed partial output path.

M3.1 still contains no physical-media write or format operation. `CMD_WRITE`, `TD_FORMAT`, `ETD_WRITE` and `ETD_FORMAT` remain forbidden.

An existing destination file must never be overwritten or truncated.

The `qualify-media-change` command is read-only and exists only to make visible-emulator media-change behavior deterministic to observe.

## Required qualification

Before M3.1 is called complete:

1. `make clean && make check && make` passes with Bebbo GCC and `-m68000`.
2. `file AmiDisk` identifies an AmigaOS loadseg()-able executable.
3. Visible FS-UAE on Motorola 68000 / Kickstart 2.04 runs `AmiDisk --version` and reports the current M3.1 build.
4. Image a known-good DF0 disk to a previously nonexistent path using `AmiDisk image-adf 0 Work:imaged.adf`.
5. Successful output reports 1760 sectors and 901120 bytes.
6. `AmiDisk adf-info Work:imaged.adf` accepts the produced image.
7. At least sector C0/H0/S0 and one non-zero-track sector from the produced ADF match the source disk bytes.
8. Host-side byte-for-byte comparison against the original source ADF is performed when the FS-UAE source disk itself is backed by a known ADF; the produced image must be identical.
9. Re-running `image-adf` with the same destination path is rejected and the existing file remains byte-for-byte unchanged.
10. Imaging with no media is rejected and leaves no output file.
11. Invalid/unavailable DF1-DF3 source handling is controlled and leaves no output file.
12. A source-media change is observed through real `trackdisk.device` state, and the imaging change-number guard remains statically verified to reject changed media and remove partial output.
13. Existing M1 probe/read-sector regression tests pass.
14. Existing M2 adf-info/adf-read-sector regression tests pass.
15. Source audit confirms no physical-media write/format command was introduced.
16. No crash, guru or hang is observed.

## M3.1a runtime procedure

Build and run the M3.1a binary in visible FS-UAE.

First verify a disk is mounted in DF0:

```text
AmiDisk probe 0
AmiDisk qualify-media-change 0
```

When AmiDisk prints:

```text
Eject or swap the disk in FS-UAE now, then press RETURN.
```

use the visible FS-UAE interface to eject DF0, or replace disk A with disk B,
then return to the Amiga shell and press RETURN.

Required observations:

- the command prints the initial media state and change number
- after eject, media becomes `absent`, or after swap it remains `present`
- the final change number differs from the initial change number
- the command prints `qualification OK: trackdisk media change observed`

For the actual imaging no-media production path, leave DF0 empty and run:

```text
AmiDisk image-adf 0 Work:no-media.adf
```

Required observations:

- controlled `no media present` imaging failure
- nonzero return status
- `Work:no-media.adf` does not exist afterward
- no crash, guru or hang

The M3.1 imaging guard itself still compares `TD_CHANGENUM` before and after
acquisition and removes the output when the value changed. M3.1a does not fake
or inject this state; it demonstrates that the same real `trackdisk.device`
change counter changes under visible FS-UAE media operations.

## Runtime qualification — 2026-09-07

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
| no-media imaging | NOT RUNTIME-OBSERVED |
| media-change during acquisition | NOT RUNTIME-OBSERVED; guard statically verified |

Host-side comparison of the copied source and generated image is byte-for-byte
identical. Both SHA-256 hashes are
`9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef`; size is
901120 bytes. No crash, guru, or hang was observed in the completed runtime.

The physical backend still contains no `CMD_WRITE`, `TD_FORMAT`, `ETD_WRITE`, or
`ETD_FORMAT`. M3.1 writes only a new ordinary ADF using exclusive `O_EXCL`
creation; existing destinations cannot be truncated.

## M3.1a runtime qualification — 2026-09-07

**M3.1 RUNTIME QUALIFICATION PASS** — GREEN. Both previously missing runtime
observations were completed in visible FS-UAE, using real DF0 eject and the
unmodified production `image-adf` command. No error codes were injected.
The earlier M3.1 results above remain historical; this run supersedes its FAIL.

### Environment and revision identity

- FS-UAE **3.2.35**, visible X11 window on the host desktop, not headless.
- CPU **Motorola 68000**, A500; emulator log confirms `CPU=68000`, no FPU/MMU/JIT.
- Kickstart **2.04 / 37.175**, ROM identified by FS-UAE as KS ROM v2.04 (A500+).
- Workbench **2.04 / 37.67**, confirmed by guest `Version`. System files were
  extracted from `amiga-os-204-workbench.adf` to a disposable host directory.
  Shell commands and libraries remained available after ejecting DF0.
- Starting HEAD: `087900c3d0e0b7955397f66ca19142e199a46727`.
- Final runtime-tested HEAD: `087900c3d0e0b7955397f66ca19142e199a46727`.
  No AmiDisk source changes were necessary; only this report and evidence changed.
- Final delivery HEAD: the qualification-report commit containing this section,
  identified by `git log -1 --format=%H -- docs/evidence/m3.1a/host-verification.txt`.
  This self-reference avoids claiming a commit can contain its own literal hash;
  the resolved final HEAD is reported in the delivery response.
- Initial `main` and `origin/main` matched; divergence `0 0`, clean worktree.
- `make clean`, `make check`, `make`, and `file AmiDisk` passed before staging.
  M0 repository, M1 static, M2 static and M3.1 static all PASS.
  Native Bebbo `m68k-amigaos-gcc` compiled every source with `-m68000`.
  `file AmiDisk`: `AmiDisk: AmigaOS loadseg()ble executable/binary`.
- The newly built binary was copied to the disposable visible guest system as
  `AmiDisk`. DF0 used known-good `original.adf`, with emulator write protection.

### Real media-change observation

The [before screenshot](evidence/m3.1a/before.png) records:

```text
Kickstart version 37.175. Workbench version 37.67
AmiDisk 0.3.0-m3.1a
Motorola 68000 / AmigaOS 2.04+
DF0: media=present write-protected=yes
DF0 before: media=present change=0
Eject or swap the disk in FS-UAE now, then press RETURN.
```

Attempt log: one invocation of `qualify-media-change 0`, one actual eject.
Initial GUI navigation did not select a media action because focus was lost.
The prompt remained pending; no RETURN was sent to AmiDisk at that point.
After explicitly activating the visible FS-UAE window, the operator automation
selected DF0 / **EJECT** in the F12 GUI menu, closed the menu, and waited more
than ten seconds before sending RETURN exactly once to the Amiga shell.
No swap, state injection, restart, or synthetic error was used.

The [after screenshot](evidence/m3.1a/media-change.png) records:

```text
DF0 after: media=absent change=1
qualification OK: trackdisk media change observed
```

`TD_CHANGENUM` changed from **0 to 1** on the same open device handle.
Media-change runtime result: **PASS**. DF0 was left empty for the next test.

### No-media production path

The [no-media screenshot](evidence/m3.1a/no-media.png) records the actual shell
commands run by `Execute nomedia`:

```text
AmiDisk probe 0
DF0: media=absent write-protected=no
AmiDisk image-adf 0 Work:no-media.adf
Imaging DF0: -> Work:no-media.adf
image-adf failed: no media present
```

The immediately captured shell `$RC` was **2**, a controlled nonzero failure.
`List Work:no-media.adf` explicitly returned:

```text
No information for "Work:no-media.adf": object not found
```

A separate guest `If EXISTS` check printed
`PASS Work:no-media.adf does not exist`. Host inspection also confirmed absence.
The destination did not exist before the test and was not created.
No-media production runtime result: **PASS**.

### Normal imaging and M1/M2 regressions

The operator automation remounted `original.adf` using the visible FS-UAE DF0
menu. `Work:m31a-regression.adf` did not previously exist; no file was overwritten.
The [regression screenshot](evidence/m3.1a/regression.png) and guest logs record:

| Command | Observed result |
| --- | --- |
| `AmiDisk probe 0` | `DF0: media=present write-protected=yes` |
| `AmiDisk image-adf 0 Work:m31a-regression.adf` | `image-adf OK: sectors=1760 bytes=901120 change=2`; RC 0 |
| `AmiDisk adf-info Work:m31a-regression.adf` | Accepted; 901120 bytes, 80 cylinders, 2 heads, 11 sectors/track, 512 bytes/sector |
| `AmiDisk adf-read-sector Work:m31a-regression.adf 0 0 0` | C0/H0/S0 read OK |
| `AmiDisk read-sector 0 0 0 0` | M1 PASS; same C0/H0/S0 bytes |
| `AmiDisk adf-info Work:m31a-regression.adf` (repeated) | M2 PASS |

Sector 0 bytes: `44 4f 53 00 e3 3d 0e 73 00 00 03 70 43 fa 00 3e`.
Additional physical/ADF reads at C10/H1/S5 both returned
`00 00 00 08 00 00 00 e5 00 00 00 07 00 00 01 e8`.
Host comparison confirmed the complete generated ADF is byte-for-byte identical
to the source, with SHA-256 (both):
`9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef`.

Additional [protection checks](evidence/m3.1a/extra.png): repeating imaging to
the existing output failed with `destination already exists`, RC 2, and the
complete file hash remained unchanged. Imaging from unavailable DF1 failed with
`cannot open source drive (could not open trackdisk.device unit)`, RC 2; guest
`If EXISTS` and host inspection confirmed no `Work:unavailable.adf` was created.

### Safety audit and stability

Source inspection and the static checks confirm:

- No active `CMD_WRITE`, `TD_FORMAT`, `ETD_WRITE`, or `ETD_FORMAT` paths exist.
- All `ad_td_do` callers use only `CMD_READ`, `TD_CHANGESTATE`, `TD_PROTSTATUS`,
  and `TD_CHANGENUM`; `trackdisk.device` remains read-only.
- `ad_image_disk_to_adf` returns `AD_IMAGE_ERR_NO_MEDIA` before destination
  creation; this is also covered by the actual no-media runtime result above.
- New destinations use `O_WRONLY | O_CREAT | O_EXCL`; existing output protection
  remains active and its ordinary existing-file case was verified at runtime.
- Partial cleanup remains present for source-read, destination-write, status,
  close, and changed-media failures. The acquisition guard still compares the
  starting and ending change numbers and discards output on a mismatch.
- M3.1 writes only to a new ordinary ADF file. No ADF-to-disk restore path exists.
- No imaging logic was changed, no physical disk-write was introduced, and no
  injected failure was used. Mid-acquisition cleanup is statically verified;
  the runtime media-change observation is the diagnostic described above.
- No crash, guru, or hang was observed throughout this qualification. The
  diagnostic's deliberate wait ended normally after the single guest RETURN;
  imaging and all scripts returned to the shell.

The final host clean/check/build/file sequence was repeated after runtime.
M0/M1/M2/M3.1 static and native Bebbo build remain PASS. Evidence, guest scripts,
version output, return codes, screenshots, emulator configuration and comparison
results are retained in [docs/evidence/m3.1a](evidence/m3.1a/).

## Status

**M3.1 RUNTIME QUALIFICATION PASS** — both required real runtime observations
(media-change and production no-media imaging) passed, with normal imaging,
M1/M2 regressions and physical-write safety audit also PASS.
