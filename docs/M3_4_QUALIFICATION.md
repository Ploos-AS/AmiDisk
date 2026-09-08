# M3.4 Direct Disk Copy runtime qualification

Status: **M3.4 RUNTIME QUALIFICATION PASS.**

## Platform, revisions and host gate

Qualification date: 2026-09-08. Visible FS-UAE 3.2.35 only, A500 / Motorola
68000, no FPU/MMU/JIT, Kickstart 2.04 / 37.175, Workbench 2.04 / 37.67.
Standard Amiga DD: 80 cylinders, 2 heads, 11 sectors/track, 512 bytes/sector,
1760 sectors, 901120 bytes.

Starting HEAD and starting origin/main:
`d63ba24fbf0653967d885e9724a8663cbe53bb84`.
The M3.3b qualified baseline is
`3fce9757e6c9aa223c450a5a9a601c8f4fb40c02`.
The qualification evidence and PASS report were committed on `main` at
`38ab24e60c1eb61cd25b703667eca2213a0ca210`. Later documentation-only
reconciliation commits may advance `main`; the tested runtime binary and the
qualification evidence remain tied to the starting qualification HEAD above.
Fetch, checkout main and ff-only pull succeeded. Initial worktree was clean,
branch main, divergence `0 0`. The sandbox required escalation for .git writes
and access to the visible X11 emulator window.

All eight initial gates passed: M0, M1, M2, M3.1, M3.2, M3.3a, M3.3b, M3.4.
Native Bebbo `m68k-amigaos-gcc` build passed using `-m68000 -noixemul`, with
warnings treated as errors. `file AmiDisk` reports
`AmiDisk: AmigaOS loadseg()ble executable/binary`.
[Initial host evidence](evidence/m3.4/initial-host.txt).

The tested binary is 31484 bytes, SHA-256
`ae864ff15e4d548eb950a9b845507d636840e15af590f8826ee893cc286f1472`.
The guest copy is byte-identical to the host build. Runtime `--version` returned
RC 0 and exactly:

```text
AmiDisk 0.3.0-m3.4
Motorola 68000 / AmigaOS 2.04+
```

## Disposable setup and evidence capture

DF0 is `/tmp/amidisk-m34/source.adf`, a new disposable copy of the known local
Workbench 2.04 ADF. DF1 is `/tmp/amidisk-m34/destination.adf`, a new disposable
copy of the known local Extras 2.04 ADF. Neither original ADF nor a real floppy
is attached. The boot system is a separate disposable directory,
`/tmp/amidisk-m34/Workbench2.0`; Work: is assigned to SYS:.

Only visible FS-UAE runs are used. A protected session runs first, closes
normally, then a writable session runs with the same pristine media. Configs
are retained as [protected](evidence/m3.4/protected.fs-uae) and
[writable](evidence/m3.4/writable.fs-uae). Startup file case duplication in the
copied system directory was corrected before the first gate. During source
eject, menu navigation was corrected visually while the qualification command
waited; no mistyped menu input is counted as a guest test result.

Scripts retain stdout in .out files and capture `$RC` immediately into .rc
files. stderr stays on the visible console, preserved in screenshots. FS-UAE
writes to disposable SDF overlays. Host backing-ADF hashes alone are therefore
not used as post-copy evidence: all proof images are produced through AmiDisk
and trackdisk from the guest-visible media.

## BEFORE evidence and mandatory rejection gates

Both initial probes returned RC 0 and `media=present`; DF1 reported
`write-protected=no` in the writable session.

| Guest image | RC | Sectors | Bytes | SHA-256 |
| --- | --- | --- | --- | --- |
| `Work:m34-source.adf` | 0 | 1760 | 901120 | `9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef` |
| `Work:m34-before.adf` | 0 | 1760 | 901120 | `3da86648e602f4e0916663d2b4893011423837b2f2b6885e421310338412c02a` |

Source and BEFORE differ. Pre-copy `verify-adf 1 Work:m34-source.adf` returned
RC 3: 1620 mismatching sectors, first C0/H0/S0, byte 4, absolute offset 4,
disk=00, adf=e3. [Baseline screenshot](evidence/m3.4/baseline-complete.png).

| Gate | Observed result | RC |
| --- | --- | --- |
| `copy-disk 0 0 ERASE-DF0` | CLI argument rejection: source and destination units must differ; backend not entered | 1 |
| `copy-disk 1 1 ERASE-DF1` | Same controlled argument rejection | 1 |
| `copy-disk 0 1 WRONG` | Confirmation mismatch; read=0 written=0 verified=0 | 2 |
| `copy-disk 0 1 ERASE-DF0` | Destination-specific confirmation mismatch; all counters zero | 2 |
| `erase-df1`, `ERASE-df1` | Case variants rejected; all counters zero | 2 each |
| `XERASE-DF1`, `ERASE-DF1X`, `ERASE-DF` | Prefix/suffix/partial variants rejected; all counters zero | 2 each |
| Source no-media | DF0 probe absent, RC 0; copy source status failure / no media; all counters zero | 2 |
| Destination no-media | DF1 probe absent, RC 0; copy destination status failure / no media; all counters zero | 2 |
| Destination write-protected | DF1 probe present/yes, RC 0; copy destination status failure / write-protected; all counters zero | 2 |

Evidence: [same-drive](evidence/m3.4/same-drive.png),
[confirmation](evidence/m3.4/wrong-confirmation.png),
[case](evidence/m3.4/case-confirmation.png),
[prefix/suffix](evidence/m3.4/prefix-suffix.png),
[partial](evidence/m3.4/partial.png),
[source no-media](evidence/m3.4/empty0.png),
[destination no-media](evidence/m3.4/empty1.png),
[write-protection](evidence/m3.4/protected.png).

After all rejection gates and remounting the same media, the control image
`Work:m34-reject-after.adf` completed with RC 0, 1760 sectors, 901120 bytes.
Full host `cmp -s` against BEFORE returned 0. DF1 remained unchanged through
the protected, same-drive, confirmation and no-media tests. Same-drive rejection
also occurs before device access in the CLI and operation-level static checks.

## Full copy and end-to-end proof

Both ready probes returned RC 0, source and destination present, destination
write-protected=no. `AmiDisk copy-disk 0 1 ERASE-DF1` completed with RC 0:

```text
copy-disk OK: sectors-read=1760 sectors-written=1760 sectors-verified=1760 bytes=901120 source-change=2 destination-change=2
```

The immediately following `verify-adf 1 Work:m34-source.adf`, with no eject or
swap, returned RC 0 and `identical sectors=1760`. AFTER imaging returned RC 0,
1760 sectors and 901120 bytes. Source and AFTER both hash to
`9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef`.
Full host `cmp -s m34-source.adf m34-after.adf` returned 0. BEFORE differs from
AFTER. [Full-copy and immediate-verify screenshot](evidence/m3.4/copy-complete.png).

Source-preservation re-imaging completed with RC 0, 1760 sectors, 901120 bytes.
`m34-source-after.adf` has the same SHA-256 as source-before, and full host `cmp`
returned 0. The complete source remained unchanged through the main direct
copy. [Source-preservation screenshot](evidence/m3.4/source-preserved.png).

All six physical `read-sector` spot checks returned RC 0 and matched pairwise:

| CHS | Identical DF0 / DF1 prefix |
| --- | --- |
| C0/H0/S0 | `44 4f 53 00 e3 3d 0e 73 00 00 03 70 43 fa 00 3e` |
| C10/H1/S5 | `00 00 00 08 00 00 00 e5 00 00 00 07 00 00 01 e8` |
| C79/H1/S10 | `00 00 00 08 00 00 06 dd 00 00 00 02 00 00 00 60` |

The main copy and all immediate end-to-end proofs completed before the
intentional interruption attempts described below.

## Media changes

Separate runtime `qualify-media-change` passed on both units while holding the
device open across GUI eject:

| Unit | Before | After | RC |
| --- | --- | --- | --- |
| DF0 | present, TD_CHANGENUM=0 | absent, TD_CHANGENUM=1 | 0 |
| DF1 | present, TD_CHANGENUM=0 | absent, TD_CHANGENUM=1 | 0 |

Both disks were remounted after these tests. After the full-copy proof and
spot checks, two separate copy attempts were deliberately interrupted by GUI
eject of one drive at a time:

| Ejected unit | Controlled result | Read | Written | Verified | Failure CHS | RC |
| --- | --- | --- | --- | --- | --- | --- |
| DF0 source | source read failed; source trackdisk.device I/O error | 54 | 54 | 54 | C2/H0/S10 | 2 |
| DF1 destination | destination write failed; destination trackdisk.device I/O error | 44 | 43 | 43 | C1/H1/S10 | 2 |

For source interruption, last successfully verified CHS was C2/H0/S9;
destination interruption last successfully verified CHS was C1/H1/S9.
Both attempts stopped and returned to the CLI without crash, guru or hang.
The source-failure path immediately returns before any following destination
write. After its return, the destination overlay hash and mtime were unchanged
across a further three-second observation, supporting the observed stop:
[no further writes](evidence/m3.4/source-abort-no-further-writes.json).
[Source abort](evidence/m3.4/interrupt0-result.png) and
[destination abort](evidence/m3.4/interrupt1-result.png) retain the diagnostics
and counters.

Exact `AD_COPY_DISK_ERR_SOURCE_CHANGED` and `AD_COPY_DISK_ERR_DEST_CHANGED`
branches were **not runtime-observed**: lower-level source-read and
destination-write errors terminated these attempts first. Real TD_CHANGENUM
changes were independently runtime-observed on both units, and both explicit
copy guards are statically verified. No test backdoor was introduced.

An interrupted copy can leave a partially overwritten destination; there is no
transactional rollback. These intentional partial attempts are not reported as
successful full copies and do not replace the earlier 1760/1760/1760 proof.
Both disposable media were remounted for the subsequent regression suite.

## Physical-write and read-back audit

[Static audit](evidence/m3.4/safety-audit.txt) confirms that `copy_disk.c` uses
only `ad_td_write_sector(&destination, ...)` for writing. It contains no
`CMD_WRITE`, `CMD_UPDATE`, `CMD_CLEAR`, `TD_FORMAT`, `ETD_WRITE` or `ETD_FORMAT`.
The backend is unchanged from the qualified M3.3b baseline. The only physical
write primitive remains `CMD_WRITE`, then `CMD_UPDATE`, then `CMD_CLEAR`, with
immediate returns after a failed write or flush.

The copy loop reads the source sector, checks both change numbers, writes the
destination via that primitive, physically reads back the destination, compares
all 512 bytes with `memcmp`, and stops on any mismatch before advancing to the
next sector. Source is never passed to the write primitive. Change guards run
before source reads, after source reads/before writes, and after verified
readbacks, as well as after the initial preflight reads. Every guard or I/O
failure closes the devices and returns. This is static verification; no
artificial production test hooks have been added. FS-UAE provided no naturally
induced read-back data mismatch during these runs; the memcmp mismatch branch
is statically verified and is not claimed as runtime-observed.

## Regressions and final host status

Runtime regressions all returned RC 0: M1 probe and read-sector; M2
`adf-info` and `adf-read-sector`; M3.1 `image-adf` (1760/901120);
M3.2 `verify-adf` (1760 identical); and M3.3a `restore-preflight` with
`NO WRITE PERFORMED`. The regression image is 901120 bytes and byte-identical
to source. A second destructive M3.3b restore was not run: M3.4 has already
runtime-proven the same qualified physical write primitive with 1760 writes and
1760 read-back verifications; M3.3b static checks remain PASS.

The final runtime host gate was rerun after qualification. M0 through M3.4
static checks, native Bebbo build and `file AmiDisk` all passed. At completion
of the qualification run, branch `main` was clean, `HEAD` equaled `origin/main`
at `d63ba24fbf0653967d885e9724a8663cbe53bb84`, and divergence was `0 0`.
The qualification report and evidence were then committed to `main` at
`38ab24e60c1eb61cd25b703667eca2213a0ca210`; that commit is the documented
M3.4 GREEN evidence point. Documentation-only reconciliation after that point
does not change the tested binary or runtime result.

## Result

**M3.4 RUNTIME QUALIFICATION PASS.** The full DF0 to DF1 copy completed with
1760 sectors read, written and read-back verified; AFTER is byte-identical to
source; source preservation, no-media, write-protect, confirmation and
same-drive safety gates passed at runtime; both drive change numbers changed
under visible eject; regressions passed; and no Amiga crash, guru or hang was
observed. Exact copy-specific SOURCE_CHANGED and DEST_CHANGED branches were
not observed because the interruption attempts reached lower-level I/O errors
first, and are recorded as such rather than inferred.
