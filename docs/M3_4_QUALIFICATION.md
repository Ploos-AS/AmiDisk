# M3.4 disk-to-disk copy qualification

Status: IMPLEMENTED, RUNTIME QUALIFICATION PENDING.

M3.4 introduces direct physical DFx-to-DFy copying for standard Amiga DD media. It reuses the M3.3b qualified trackdisk sector-write primitive; M3.4 does not add another raw physical write path.

## Safety invariants

- source and destination units must be different (`DFx != DFy`)
- source and destination must both contain readable media
- destination must not be write-protected
- destructive confirmation must exactly match `ERASE-DF<destination-unit>`
- source and destination `TD_CHANGENUM` values are captured before copying and checked throughout the operation
- source is read one 512-byte sector at a time
- destination writes go only through `ad_td_write_sector`
- the qualified write primitive performs `CMD_WRITE`, `CMD_UPDATE`, then `CMD_CLEAR`
- every destination sector is immediately read back and compared before the next sector proceeds
- first error stops the copy; a failure after writes have started can leave a partially modified destination
- no `TD_FORMAT`, `ETD_WRITE` or `ETD_FORMAT` path is introduced

## Host gate

Require:

- M0 repository checks: PASS
- M1 static checks: PASS
- M2 static checks: PASS
- M3.1 static checks: PASS
- M3.2 static checks: PASS
- M3.3a static checks: PASS
- M3.3b static checks: PASS
- M3.4 static checks: PASS
- native Bebbo build using `m68k-amigaos-gcc` and `-m68000`: PASS
- `file AmiDisk` identifies an AmigaOS loadseg executable

Expected version:

```text
AmiDisk 0.3.0-m3.4
Motorola 68000 / AmigaOS 2.04+
```

## Visible FS-UAE runtime qualification

Use visible FS-UAE only and disposable destination media.

Recommended setup:

- source in DF0
- writable disposable destination in DF1
- source and destination initially contain different data

The command under qualification is:

```text
AmiDisk copy-disk 0 1 ERASE-DF1
```

Required negative tests before the successful copy:

1. source and destination are the same unit: reject before writing
2. wrong confirmation: controlled failure, zero sectors written
3. destination-specific wrong confirmation such as `ERASE-DF0` for DF1: controlled failure, zero sectors written
4. no source media: controlled failure, zero sectors written
5. no destination media: controlled failure, zero sectors written
6. write-protected destination: controlled failure, zero sectors written

For the successful end-to-end test:

1. image DF0 to a source reference ADF before the copy
2. image DF1 to a BEFORE ADF before the copy
3. prove BEFORE differs from the source reference
4. run `copy-disk 0 1 ERASE-DF1`
5. require RC 0
6. require 1760 sectors read
7. require 1760 sectors written
8. require 1760 sectors read-back verified
9. require 901120 bytes written
10. image DF1 to an AFTER ADF
11. require AFTER SHA-256 equals source-reference SHA-256
12. require full byte-for-byte equality between source reference and AFTER
13. require BEFORE differs from AFTER

Also spot-check at least:

- C0/H0/S0
- C10/H1/S5
- C79/H1/S10

## Media-change qualification

Runtime-observe `TD_CHANGENUM` on both drives. If practical with disposable media, eject or swap source and destination independently during separate copy attempts and require a controlled stop without crash/guru/hang.

If a timing-sensitive copy ends through a lower-level controlled I/O failure before the explicit media-change result is reached, record that honestly. Do not add production test backdoors solely to force a branch.

## Regression requirements

After M3.4 runtime testing, re-run M1, M2, M3.1, M3.2, M3.3a and M3.3b relevant runtime checks. In particular, verify that ADF restore still produces 1760 written and 1760 verified sectors using the same qualified write primitive.

## PASS criteria

M3.4 is GREEN only when all of the following are documented:

- host/static gates PASS
- native 68000 build PASS
- negative safety gates behave as specified
- actual DFx-to-DFy copy completes with 1760/1760/1760 sectors read/written/verified
- AFTER image is byte-identical to the source reference
- BEFORE image was different, proving that the destination was actually modified
- source and destination media-change protection is demonstrated or its runtime limitation is accurately documented with static guards verified
- regressions PASS
- no Amiga guru/crash/hang

Do not proceed to later M3 work until M3.4 is qualified and documented as PASS.
