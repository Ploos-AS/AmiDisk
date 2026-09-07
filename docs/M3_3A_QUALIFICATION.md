# M3.3a qualification - read-only restore preflight

## Scope

M3.3a validates the safety checks needed before any future restore capability. This milestone remains read-only.

Command:

```text
AmiDisk restore-preflight <path> <unit> <confirmation>
```

The command validates the standard ADF source and destination drive, checks media presence and write protection, performs a read-only destination sector test, checks the trackdisk media change number before and after, and reports `NO WRITE PERFORMED`.

## Required qualification

1. `make clean && make check && make` passes with Bebbo GCC and `-m68000`.
2. `file AmiDisk` reports an AmigaOS executable.
3. Visible FS-UAE on Motorola 68000 / Kickstart 2.04 reports version `0.3.0-m3.3a`.
4. Incorrect confirmation is rejected cleanly.
5. Missing and wrong-size ADF inputs are rejected cleanly.
6. No-media destination is rejected cleanly.
7. Write-protected destination media is rejected cleanly.
8. Valid ADF plus writable media and the exact confirmation passes preflight.
9. Successful preflight reports 901120 source bytes and stable media state/change number.
10. Destination bytes are checked before and after successful preflight and remain identical.
11. Existing M1, M2, M3.1 and M3.2 regressions pass.
12. Source audit confirms M3.3a added no physical write or format operation.
13. No crash, guru or hang is observed.


## Runtime qualification — 2026-09-07

### Environment and revisions

- Starting HEAD: `5ab22a6ded84d91459506aa46c301fe85e36b4db` on `main`.
  Fetch, checkout main and ff-only pull completed; HEAD and origin/main matched,
  divergence `0 0`, clean worktree. This is also the runtime-tested source revision.
- Visible FS-UAE 3.2.35, A500, Motorola 68000, no FPU/MMU/JIT.
- Guest Version reports Kickstart 2.04 / 37.175 and Workbench 2.04 / 37.67.
- Standard DD: 80 cylinders, 2 heads, 11 sectors/track, 512 bytes/sector,
  1760 sectors, 901120 bytes.
- Directory-backed disposable Workbench system at `/tmp/amidisk-m33a/Workbench2.0`,
  `Work:` assigned to `SYS:`. DF0 uses only `/tmp/amidisk-m33a/test.adf`.
  No physical floppy device is attached. No restore or format command was run.
- Initial session uses `uae_floppy_write_protect = true`. After protected and
  no-media tests, that qualification window was closed and visible FS-UAE was
  relaunched with the same test ADF and `uae_floppy_write_protect = false`.
  Guest startup only assigns Work, adds the command path and prints Version.
  Protection was confirmed through actual guest probes in both sessions.
- [Configurations and emulator excerpts](evidence/m3.3a/emulator-excerpts.txt):
  [protected](evidence/m3.3a/qualification.fs-uae),
  [writable](evidence/m3.3a/writable.fs-uae).
- Native Bebbo `m68k-amigaos-gcc (GCC) 6.5.0b 20260807212032`;
  compilation uses `-m68000 -noixemul`.
- Initial clean/check/build: M0 repository, M1, M2, M3.1, M3.2, M3.3a static
  checks all PASS; native build PASS. `file AmiDisk` reports
  `AmiDisk: AmigaOS loadseg()ble executable/binary`.
- Runtime version: `AmiDisk 0.3.0-m3.3a`, `Motorola 68000 / AmigaOS 2.04+`.
  See [environment/version](evidence/m3.3a/start.png) and
  [baseline](evidence/m3.3a/baseline-progress.png).

### Fixtures and destination baseline

Test disk and source reference are separate copies of the known-good M3.1/M3.2
standard Workbench ADF. Both are 901120 bytes, SHA-256
`9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef`.
Short/long fixtures are the previous controlled fixtures, 901119/901121 bytes;
the missing path does not exist. [Fixture hashes](evidence/m3.3a/fixture-hashes.json)
record sizes, hashes and the runtime binary hash before preflight tests.

Initial `probe 0`: `media=present write-protected=yes`, RC 0.
`read-sector 0 0 0 0`: read OK, RC 0, prefix:

```text
44 4f 53 00 e3 3d 0e 73 00 00 03 70 43 fa 00 3e
```

Before any preflight test, `image-adf 0 Work:m33a-before.adf` completed through
the qualified M3.1 path: `sectors=1760 bytes=901120 change=0`, RC 0.
The destination filename was new. The full before image remains in the disposable
host guest-directory; OS media images are not committed to the repository.

### Observed rejection results

Guest scripts capture `$RC` immediately after each invocation, before Type/Echo
can replace it. Individual `m33a-*.out` and `m33a-*.rc` files are retained in
[evidence](evidence/m3.3a/). stderr remains on the visible console and is retained
in screenshots. All preflight invocations below use the production binary.

| Test | Actual result | RC |
| --- | --- | --- |
| `WRONG` for DF0 | confirmation rejected | 2 |
| `ERASE-DF1` for DF0 | confirmation rejected | 2 |
| `erase-df0`, `ERASE-df0` | confirmation rejected | 2 each |
| `XERASE-DF0`, `ERASE-DF0X`, `ERASE-DF` | confirmation rejected | 2 each |
| Missing `Work:does-not-exist.adf`, exact confirmation | source ADF rejected (cannot open ADF) | 2 |
| `Work:short.adf`, 901119 bytes | source ADF rejected (not a standard 880 KiB ADF) | 2 |
| `Work:long.adf`, 901121 bytes | source ADF rejected (not a standard 880 KiB ADF) | 2 |
| DF0 ejected via visible GUI | probe: media=absent write-protected=no | 0 |
| Valid source, exact confirmation, DF0 empty | no destination media present | 2 |
| Protected DF0 probe | media=present write-protected=yes | 0 |
| Valid source, exact confirmation, protected DF0 | destination media is write-protected | 2 |
| Unit 4, `ERASE-DF4` | CLI: unit must be 0..3 | 1 |
| Unit 1, `ERASE-DF1` | cannot open destination drive (could not open trackdisk.device unit) | 2 |

Every invocation entering the preflight command reports `NO WRITE PERFORMED`,
including failures. Unit 4 is rejected by CLI before command/device dispatch and
therefore prints only its CLI diagnostic. DF1 is disabled in this configuration;
its device-open failure is controlled, not an observed DF1 no-media result.

Evidence: [wrong/wrong-unit/lowercase](evidence/m3.3a/reject4.png),
[mixed-case/prefix/suffix](evidence/m3.3a/reject9.png),
[partial/missing](evidence/m3.3a/reject12.png),
[missing/short/long](evidence/m3.3a/reject15.png),
[unit boundaries and write protection](evidence/m3.3a/reject-final.png),
[actual no-media](evidence/m3.3a/no-media.png).

No preflight invocation creates an image/output destination or changes source
ADF data. Redirected diagnostic/RC files are qualification evidence, not disk
outputs. The full before/after comparison below covers the destination content
across these tests, including the successful writable test.

### Successful writable preflight and mandatory read-only proof

The same test ADF was mounted without emulator write protection. Actual probe:
`DF0: media=present write-protected=no`, RC 0.

```text
AmiDisk restore-preflight Work:reference.adf 0 ERASE-DF0
restore-preflight OK: source=901120 bytes media=present write-protected=no change=0
NO WRITE PERFORMED
```

RC **0**, runtime-observed. See [success](evidence/m3.3a/success.png).
The production success line summarizes source size, media, protection and final
change number. Accepted exact confirmation, readable sector 0 and equal start/end
numbers are prerequisites for that success in the inspected implementation;
they are not separately printed CLI fields. No instrumentation was added.

RC 0 means only **SAFE TO PROCEED TO A FUTURE RESTORE IMPLEMENTATION**.
It does not mean that restore was performed. M3.3b was not started.

The next AmiDisk invocation was `image-adf 0 Work:m33a-after.adf`, using another
new filename. It completed with `sectors=1760 bytes=901120 change=0`, RC 0.
See [completed imaging](evidence/m3.3a/after-complete.png).

| Image | Bytes | SHA-256 |
| --- | --- | --- |
| `Work:m33a-before.adf` | 901120 | `9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef` |
| `Work:m33a-after.adf` | 901120 | `9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef` |

Host comparison of the complete byte arrays also passed, independently of hash
equality. [Before/after evidence](evidence/m3.3a/before-after.json).
Comparison was gated on imaging completion and RC 0; an interim hash taken while
the after file was still growing was not treated as a completed image result.
**Destination byte-identical before/after: PASS.**

### Media-change guard: runtime and static evidence

After successful preflight and completed after imaging, `qualify-media-change 0`
held its device open while DF0 was ejected using the visible FS-UAE GUI.
After allowing the emulator to register eject, RETURN was sent:

```text
DF0 before: media=present change=0
Eject or swap the disk in FS-UAE now, then press RETURN.
DF0 after: media=absent change=1
qualification OK: trackdisk media change observed
```

RC 0. [Before](evidence/m3.3a/change-before.png),
[GUI eject menu](evidence/m3.3a/change-menu.png),
[completion and RC](evidence/m3.3a/change-complete.png).
**Actual trackdisk TD_CHANGENUM runtime change: PASS (0 → 1).**

Preflight reads start change number, reads destination sector 0, reads end
change number, closes resources, and rejects unequal numbers before success.
The stable path ran successfully. **Preflight media-changed rejection/end-guard
branch is statically verified, not runtime-observed.** Preflight has no pause and
completes too quickly for practical manual GUI swapping. Production code was not
altered to force that branch. The separate real device test is not claimed as
runtime coverage of the preflight-specific rejection branch.

### Physical-write safety audit

[Audit evidence](evidence/m3.3a/safety-audit.txt): full production source scan
contains no `CMD_WRITE`, `TD_FORMAT`, `ETD_WRITE` or `ETD_FORMAT` token.
Occurrences elsewhere are documentation and forbidden-token static checks.
All trackdisk DoIO call sites use only `CMD_READ`, `TD_CHANGESTATE`,
`TD_PROTSTATUS` and `TD_CHANGENUM`; no numeric/alternate physical write path exists.

`restore_preflight.c` only validates arguments/exact strcmp confirmation, opens
the source through the read-only ADF backend, queries destination status, reads
sector 0 and checks change numbers. No sector writes, track formats, source
modification, writable source open, or file-writing operation occurs there.
ADF open remains `fopen(path, "rb")`. Image/ADF/trackdisk code is unchanged from
qualified M3.1a revision `087900c3d0e0b7955397f66ca19142e199a46727`.
M3.1 new-output creation retains `O_WRONLY | O_CREAT | O_EXCL` and its existing-file
protection. The expected new image files are the only AmiDisk data-write path.

### Execution notes

No crash, guru or hang was observed. Completed commands returned to the shell;
the media-change diagnostic intentionally waited for RETURN.
The copied initial guest startup ran the old M3.2 read-only script against the
new binary before the requested baseline script. Its verify tests do not write
media. A first typed baseline command lost characters and returned an ordinary
unknown-command error; corrected key timing allowed the exact script to run.
During no-media menu navigation, F12 briefly reopened the menu after eject;
that input did not reach the shell. The menu was closed and the intended script
then ran correctly. These input/setup events are not claimed as qualification
results. Existing unrelated emulator windows were left alone.

### Regressions and final integrity

The known-good test ADF was remounted through the visible GUI after the change
check. All six regression commands returned RC 0:

| Command | Runtime result |
| --- | --- |
| `probe 0` | media=present write-protected=no |
| `read-sector 0 0 0 0` | read OK, original sector prefix |
| `adf-info Work:reference.adf` | exact standard 901120-byte DD geometry |
| `adf-read-sector Work:reference.adf 0 0 0` | read OK, same prefix as disk |
| `verify-adf 0 Work:reference.adf` | identical sectors=1760 change=2 |
| `image-adf 0 Work:m33a-regression.adf` | sectors=1760 bytes=901120 change=2 |

[Regression screenshot](evidence/m3.3a/regression.png),
[probe and initial results](evidence/m3.3a/regression-progress.png).
Host comparison confirms the new regression image also equals the reference,
before image and after image byte-for-byte. Final checks confirm mounted ADF,
reference, short and long fixtures and guest binary have unchanged hashes; the
missing path remains absent. [Final hashes](evidence/m3.3a/final-hashes.json).
**M1, M2, M3.1 and M3.2 runtime regressions: PASS.**

### Final host gate and delivery

The final `make clean`, `make check`, `make`, `file AmiDisk` gate passes:
M0 repository checks, M1, M2, M3.1, M3.2, M3.3a static checks all PASS;
native Bebbo build PASS. See [host verification](evidence/m3.3a/host-verification.txt).
The rebuilt binary matches the runtime-tested guest binary byte-for-byte:
SHA-256 `6ecf93a515e779e6e7d0ce8cdd8178c82d26f09f0f707b3fd1af5eb41a085cdf`.
No production code or test weakening was needed. Only this report and its
evidence are delivered.

The source HEAD at the final build gate is the starting revision. Final delivery
HEAD is the commit containing this report and evidence, resolvable with
`git log -1 --format=%H -- docs/evidence/m3.3a/host-verification.txt`.
Its literal hash and post-push main/origin/main state are given in the delivery
response, avoiding a self-referential hash within its own commit. Delivery is
to main without a temporary qualification branch.

## Status

**M3.3a RUNTIME QUALIFICATION PASS.** Successful writable-media preflight with
RC 0 and `NO WRITE PERFORMED` is runtime-observed; complete destination images
are proven byte-identical before/after. Required rejection tests, real no-media,
write-protected rejection, true trackdisk media change, regressions, static gates,
native build and physical-write safety audit pass. No crash/guru/hang observed.

The preflight-specific changed-number rejection branch remains statically
verified, not runtime-observed. M3.3a remains read-only. M3.3b was not started.
