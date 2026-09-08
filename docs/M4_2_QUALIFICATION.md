# M4.2 Partial Recovery Imaging qualification

Status: IMPLEMENTED. HOST/NATIVE AND RUNTIME QUALIFICATION PENDING.

Starting qualified baseline: `d550df1b0ef71867329797f7e5f1fb1ce083f431` (M4.1 GREEN).

## Scope

M4.2 adds a full-geometry recovery workflow built on the qualified M4.1
`ad_recovery_read_sector()` primitive. The physical source remains strictly
read-only.

CLI:

```text
AmiDisk recover-image <unit> <image-path> <map-path> <attempts>
```

The image artifact is always ADF-shaped (901120 bytes) when the operation
returns complete or partial. This does **not** mean every byte was recovered.
The companion machine-readable map is authoritative for sector state.

## Sector states

Every one of the 1760 standard DD sectors is represented in the map as one of:

- `GOOD`: sector data was successfully recovered and written to the image.
- `BAD`: the configured retry budget was genuinely exhausted; the image sector
  contains zero placeholder bytes and must not be described as recovered data.
- `UNREAD`: the sector was not trusted/read because acquisition was aborted by
  a source/media-identity failure; the image sector contains zero placeholder
  bytes and must not be described as recovered data.

The image therefore remains geometry-compatible while the map prevents
placeholder bytes from being confused with recovered evidence.

## Map format v1

The tab-separated map begins with:

```text
AMIDISK_RECOVERY_MAP	1
geometry	80	2	11	512	1760	901120
source_unit	N
retry_budget	N
placeholder_byte	00
columns	index	cylinder	head	sector	state	attempts	td_result
```

There is then exactly one row per sector in linear disk order.

## Safety and acquisition invariants

- DF0-DF3 only.
- retry budget 1-16.
- image and map paths must differ.
- neither output may already exist; both are created with `O_EXCL`.
- the source is status-checked before output creation.
- a long-lived monitor handle captures the starting `TD_CHANGENUM`.
- source identity is checked before and after every M4.1 recovery-read call.
- a GOOD sector is emitted only after both the recovery read and whole-acquisition
  media-identity guard succeed.
- retry exhaustion emits `BAD` plus a zero placeholder and acquisition continues.
- a media/source failure emits `UNREAD` for the current and all remaining sectors,
  fills their image positions with zero placeholders, and returns PARTIAL.
- complete or partial results have exactly 1760 image sectors / 901120 bytes and
  a complete sector map.
- local image/map write failures are hard errors; incomplete/unreliable output
  files are removed.
- no physical write/format command is reachable from `recovery_image.c`.

## Return/CLI semantics

- complete all-GOOD acquisition: operation OK, CLI RC 0.
- any BAD or UNREAD state: operation PARTIAL, CLI RC 3, with an explicit
  `consult map` diagnostic.
- argument/source/output failures before a valid recovery artifact exists:
  controlled error, CLI RC 1 or 2 as appropriate.

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

Require:

- M0 through M4.2 static gates PASS.
- native Bebbo build PASS with `m68k-amigaos-gcc -m68000 -noixemul`.
- AmigaOS loadseg executable.
- version `AmiDisk 0.4.0-m4.2`.

## Visible FS-UAE runtime qualification

Use visible FS-UAE only on the qualified Motorola 68000 / AmigaOS 2.04+
baseline and disposable/known-good DD source media.

### 1. Healthy full acquisition

Run with a new image path and map path, for example:

```text
AmiDisk recover-image 0 Work:m42-good.adf Work:m42-good.map 4
```

Require RC 0 and:

- good=1760
- bad=0
- unread=0
- placeholders=0
- sectors=1760
- bytes=901120

Verify the resulting image against DF0 with `verify-adf`; require RC 0 and 1760
identical sectors. Host-side hash/full `cmp` against a normal M3.1 image of the
same unchanged source is strongly preferred.

Inspect the map and require:

- correct v1 header/geometry/source/retry metadata;
- exactly 1760 sector rows;
- every sector row is GOOD;
- attempts are at least 1 and no more than the configured budget;
- no BAD/UNREAD rows on the healthy run.

### 2. Exclusive destination protection

Re-run using an existing image path and separately an existing map path. Require
controlled rejection with neither existing file modified. Test image path == map
path and require argument rejection.

### 3. Retry budget boundaries and unit gate

Test attempts 0 and 17 and unit 4. Require deterministic CLI rejection before
recovery acquisition begins.

### 4. No-media preflight

With DF0 visibly ejected, run `recover-image` with new output names. Require a
controlled no-media error and no recovery image/map artifacts created.

### 5. Partial UNREAD acquisition through real media interruption

On a disposable healthy source, start `recover-image` and visibly eject/swap DF0
while acquisition is in progress if practical. Require:

- controlled PARTIAL/RC 3 if the operation has already begun output acquisition;
- image remains exactly 901120 bytes;
- map has exactly 1760 sector rows;
- GOOD rows only precede the interruption;
- current/remaining untrusted sectors are UNREAD with placeholder sectors;
- GOOD + BAD + UNREAD = 1760;
- placeholder count = BAD + UNREAD;
- no sector after media-identity loss is claimed GOOD;
- no crash/guru/hang.

If a lower-level timing outcome prevents the exact mid-operation path from being
observed, document it precisely and do not add a production timing backdoor.
Real `TD_CHANGENUM` behavior is already qualified in M4.1, but the M4.2
whole-acquisition guard must remain statically verified.

### 6. BAD/retry-exhaustion path

If a naturally unreadable sector is available, acquire it with a known retry
budget and require a BAD map row whose attempts equals the exhausted budget,
with a zero placeholder in the image at that CHS while acquisition continues.

If no naturally bad sector is available, mark BAD emission as statically
verified but not runtime-observed. Do not fabricate bad sectors with production
hooks.

### 7. Physical read-only audit

Confirm `src/operations/recovery_image.c` contains none of:

- `CMD_WRITE`
- `CMD_UPDATE`
- `CMD_CLEAR`
- `TD_FORMAT`
- `ETD_WRITE`
- `ETD_FORMAT`
- `ad_td_write_sector`

The only data acquisition call from M4.2 must be the M4.1
`ad_recovery_read_sector()` path.

### 8. Regressions

Rerun representative M1-M4.1 paths, including `recover-read`. M3.3b/M3.4
physical-write paths need not be destructively rerun because M4.2 does not alter
them; their static gates must remain PASS.

## PASS criteria

M4.2 may be marked GREEN only when host/static/native qualification passes and
visible runtime proves a complete healthy recovery image plus authoritative map,
exclusive output protection, no-media behavior, source read-only safety and
regressions. Mid-operation UNREAD behavior should be runtime-observed when
practical without test backdoors; BAD remains static-only if no naturally
unreadable sector exists.

Do not begin M4.3 during M4.2 qualification.
