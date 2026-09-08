# M4.1 Recovery Read Foundation qualification

Status: IMPLEMENTED. RUNTIME QUALIFICATION PENDING.

Starting qualified baseline: `4ae68da65ba6db3d672b678ec0406918a6e79d14` (M3 GREEN reconciliation).

## Scope

M4.1 introduces only the bounded recovery-read primitive and its per-sector evidence record. It does not create partial ADF files yet and it does not write to physical media.

The API is `ad_recovery_read_sector()` in `src/operations/recovery_read.[ch]`.

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
make clean
make check
make
file AmiDisk
./tools/check_m4_1.py
```

Expected:

- M0 through M3.4 static gates PASS.
- M4.1 static checks PASS.
- native Bebbo build PASS with `m68k-amigaos-gcc`, `-m68000`, `-noixemul`.
- `file AmiDisk` reports an AmigaOS loadseg executable.
- version is `AmiDisk 0.4.0-m4.1`.

## Runtime qualification strategy

M4.1 has no user-facing recovery command yet; the public CLI integration belongs to the next M4 step once the retry primitive has passed host/native qualification. Runtime qualification will therefore be completed together with that integration, in visible FS-UAE only.

The eventual runtime evidence must include:

1. healthy-sector recovery with retry budget 1 and a correct 512-byte result;
2. healthy-sector recovery with a larger retry budget while proving attempts=1;
3. invalid retry budgets 0 and >16 rejected before device I/O;
4. no-media controlled rejection;
5. real `TD_CHANGENUM` observation under eject/swap;
6. if a naturally failing sector is available, exhausted retry accounting matching the configured budget;
7. if no naturally failing sector is available, the exhausted branch remains statically verified and must not be claimed as runtime-observed;
8. M1-M3 regressions remain green;
9. source remains read-only and no guru/crash/hang is observed.

Do not add a production test backdoor merely to manufacture bad sectors.

## Safety audit

`src/operations/recovery_read.c` must contain none of:

- `CMD_WRITE`
- `CMD_UPDATE`
- `CMD_CLEAR`
- `TD_FORMAT`
- `ETD_WRITE`
- `ETD_FORMAT`
- `ad_td_write_sector`

M4.1 may only observe/read the source disk.
