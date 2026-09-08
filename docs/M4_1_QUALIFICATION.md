# M4.1 Recovery Read Foundation qualification

Status: M4.1 RUNTIME QUALIFICATION PASS.

Starting qualified baseline: `4ae68da65ba6db3d672b678ec0406918a6e79d14` (M3 GREEN reconciliation).

## Scope

M4.1 introduces the bounded recovery-read primitive, its per-sector evidence record, and a thin CLI qualification surface. It does not create partial ADF files yet and it does not write to physical media.

The API is `ad_recovery_read_sector()` in `src/operations/recovery_read.[ch]`.

The CLI is:

```text
AmiDisk recover-read <unit> <cylinder> <head> <sector> <attempts>
```

Required properties:

- DF0-DF3 only.
- standard DD CHS range only.
- caller supplies a retry budget from 1 through `AD_RECOVERY_MAX_ATTEMPTS` (16).
- the same CHS is retried through the existing qualified `ad_td_read_sector()` primitive.
- attempts are counted deterministically.
- source no-media is a distinct controlled result.
- `TD_CHANGENUM` is captured before recovery reads and checked during the retry loop and after a successful/final read path.
- media replacement aborts the recovery read rather than mixing evidence from different media.
- the record preserves CHS, attempts, start/end change number and the last underlying trackdisk result.
- no physical write/format primitive is reachable from this module.

## Host qualification

Run:

```sh
git fetch origin
git checkout main
git pull --ff-only origin main
git status --short
git rev-parse HEAD
git rev-parse origin/main
git rev-list --left-right --count origin/main...HEAD
make clean
make check
make
file AmiDisk
```

Expected:

- M0 through M3.4 static gates PASS.
- M4.1 static checks PASS.
- native Bebbo build PASS with `m68k-amigaos-gcc`, `-m68000`, `-noixemul`.
- `file AmiDisk` reports an AmigaOS loadseg executable.
- version is `AmiDisk 0.4.0-m4.1`.

## Visible FS-UAE runtime qualification

Use visible FS-UAE only on the qualified Motorola 68000 / AmigaOS 2.04+ baseline. A normal standard-DD test disk in DF0 is sufficient for the mandatory success and no-media tests.

### 1. Version and source baseline

Run:

```text
AmiDisk --version
AmiDisk probe 0
AmiDisk read-sector 0 0 0 0
```

Require version `0.4.0-m4.1`, media present, and a successful baseline sector read.

### 2. Retry budget 1

Run:

```text
AmiDisk recover-read 0 0 0 0 1
```

Require RC 0, `attempts=1`, unchanged media change number, and the same 16-byte prefix as `read-sector 0 0 0 0`.

### 3. Larger retry budget on healthy media

Run:

```text
AmiDisk recover-read 0 10 1 5 16
```

Require RC 0 and `attempts=1`. A healthy sector must not consume the remaining retry budget. Compare the printed prefix with:

```text
AmiDisk read-sector 0 10 1 5
```

They must match.

### 4. Retry budget boundaries

Run:

```text
AmiDisk recover-read 0 0 0 0 0
AmiDisk recover-read 0 0 0 0 17
```

Require controlled CLI rejection, RC 1, and `attempts must be 1..16`. These values must be rejected before the recovery backend/device path is entered.

Also run the legal maximum:

```text
AmiDisk recover-read 0 79 1 10 16
```

Require RC 0 on healthy media.

### 5. CHS and unit ranges

Run invalid cases such as:

```text
AmiDisk recover-read 4 0 0 0 1
AmiDisk recover-read 0 80 0 0 1
AmiDisk recover-read 0 0 2 0 1
AmiDisk recover-read 0 0 0 11 1
```

Unit 4 must be rejected by CLI with RC 1. Invalid CHS must fail in a controlled way with no crash/guru/hang and no physical write.

### 6. No-media

Eject DF0 and confirm:

```text
AmiDisk probe 0
```

reports media absent. Then run:

```text
AmiDisk recover-read 0 0 0 0 4
```

Require controlled RC 2, no-media result, zero attempts, and no crash/guru/hang. Remount the source afterward.

### 7. Real media-change observation

Run:

```text
AmiDisk qualify-media-change 0
```

Eject/swap while the command waits. Require `TD_CHANGENUM` to change.

If practical, attempt to eject/swap during a `recover-read` invocation. A controlled lower-level read/status abort is acceptable if it occurs before the explicit `AD_RECOVERY_READ_ERR_MEDIA_CHANGED` branch. Document the exact result honestly. Do not add a production test backdoor to force timing or a bad sector.

### 8. Exhausted retry branch

If a naturally unreadable sector is available, run recovery with a known budget, for example:

```text
AmiDisk recover-read 0 C H S 4
```

Require controlled RC 2 and `attempts=4` when all four reads genuinely fail without a media replacement.

If no naturally unreadable sector is available, record the exhausted branch as statically verified but not runtime-observed. This does not block M4.1 PASS provided the bounded-loop implementation, range checks, no-media behavior and real media-change semantics are qualified.

### 9. Read-only safety audit

Statically confirm `src/operations/recovery_read.c` contains none of:

- `CMD_WRITE`
- `CMD_UPDATE`
- `CMD_CLEAR`
- `TD_FORMAT`
- `ETD_WRITE`
- `ETD_FORMAT`
- `ad_td_write_sector`

The only sector I/O primitive in the retry loop must be one shared call site to `ad_td_read_sector()`.

### 10. Regression suite

Run representative M1-M3 regressions:

```text
AmiDisk probe 0
AmiDisk read-sector 0 0 0 0
AmiDisk adf-info <known-good.adf>
AmiDisk adf-read-sector <known-good.adf> 0 0 0
AmiDisk image-adf 0 <new-output.adf>
AmiDisk verify-adf 0 <matching.adf>
```

M3.3a preflight should also remain read-only and PASS against a valid disposable destination setup. Destructive M3.3b/M3.4 reruns are optional because M4.1 does not touch the write path; their static gates must still PASS.

## PASS criteria

M4.1 may be marked GREEN only when:

- host/static/native build is green;
- `recover-read` works on healthy media with budgets 1 and 16;
- a healthy sector reports `attempts=1` even with a larger budget;
- retry budgets 0 and 17 are rejected;
- no-media is runtime-observed as controlled failure;
- real `TD_CHANGENUM` change is runtime-observed;
- exhausted retry behavior is either runtime-observed on a naturally failing sector or explicitly documented as static-only;
- the recovery module remains physically read-only;
- M1-M3 regressions remain green;
- no crash, guru or hang is observed.

Do not add a production test backdoor merely to manufacture bad sectors or media-change timing.

## Completed runtime evidence

Starting HEAD, origin/main and divergence were respectively
`d4049458774c85ae4644ee51f5b176f58a9bae8c`, the same, and `0 0`; the starting
worktree was clean. The final commit is recorded below. FS-UAE 3.2.35 was used
visibly with Motorola 68000, Kickstart 37.175 and Workbench 37.67. DF0 held a
known-good disposable standard DD Workbench disk; no original or valuable floppy
was used, and the source remained read-only throughout.

The host gate passed M0, M1, M2, M3.1, M3.2, M3.3a, M3.3b, M3.4 and M4.1
static checks. Native `m68k-amigaos-gcc` with `-m68000 -noixemul` passed and
`file AmiDisk` reported an AmigaOS `loadseg()` executable. The guest version was
`AmiDisk 0.4.0-m4.1` / `Motorola 68000 / AmigaOS 2.04+`.

Healthy recovery reads both returned RC 0 and attempts=1. C0/H0/S0 printed
`44 4f 53 00 e3 3d 0e 73 00 00 03 70 43 fa 00 3e`, identical to normal
`read-sector`. C10/H1/S5 with budget 16 also returned attempts=1 and printed
`00 00 00 08 00 00 00 e5 00 00 00 07 00 00 01 e8`, identical to normal
`read-sector`. The legal maximum C79/H1/S10 with budget 16 returned RC 0 and
attempts=1.

Retry budgets 0 and 17 were deterministically rejected with RC 1 and
`attempts must be 1..16`. Unit 4 was rejected with RC 1 and `unit must be
0..3`; C80, H2 and S11 were rejected with RC 2 and `invalid recovery read
argument`. These validations occur before the recovery device path. No
naturally unreadable sector was available, so `AD_RECOVERY_READ_ERR_EXHAUSTED`
is statically verified but not runtime-observed.

After a visible DF0 eject, `probe 0` reported `media=absent`; a valid
`recover-read 0 0 0 0 4` returned RC 2, `no media present`, attempts=0, with no
crash, guru or hang. A separate visible `qualify-media-change 0` run reported
`DF0 before: media=present change=0` and `DF0 after: media=absent change=1`,
RC 0, `qualification OK: trackdisk media change observed`. An attempted
recovery after that eject reached the controlled no-media result; the exact
`AD_RECOVERY_READ_ERR_MEDIA_CHANGED` branch was not claimed as runtime-observed.

The read-only safety audit found no `CMD_WRITE`, `CMD_UPDATE`, `CMD_CLEAR`,
`TD_FORMAT`, `ETD_WRITE`, `ETD_FORMAT` or `ad_td_write_sector` in
`recovery_read.c`; the retry loop has one shared `ad_td_read_sector` path.

Representative regressions passed in visible FS-UAE: M1 probe/read-sector, M2
`adf-info`/`adf-read-sector`, M3.1 `image-adf` (1760 sectors/901120 bytes),
M3.2 `verify-adf` (1760 identical sectors), and M3.3a
`restore-preflight` (`NO WRITE PERFORMED`). M3.3b and M3.4 static gates remain
PASS; no destructive rerun was needed. No crash, guru or hang occurred and no
production test backdoor was introduced.

Runtime evidence is retained under `docs/evidence/m4.1/`.

## Final result

**M4.1 RUNTIME QUALIFICATION PASS.**

## Final report

Record starting HEAD, final HEAD, origin/main, divergence, worktree, all static gates, native build, version, healthy-read results, attempts counts, invalid-budget results, invalid-range results, no-media result, media-change evidence, exhausted-branch status, read-only audit, regressions and crash/guru/hang status. Commit the evidence/report on `main`, push it, and leave the worktree clean with divergence `0 0`.
