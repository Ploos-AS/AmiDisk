# M3.3b restore qualification

Status: IMPLEMENTED, RUNTIME QUALIFICATION PENDING.

M3.3b is the first AmiDisk milestone that can modify physical floppy media. Runtime testing must therefore use only disposable media in visible FS-UAE.

## Required host gates

- M0 static PASS
- M1 static PASS
- M2 static PASS
- M3.1 static PASS
- M3.2 static PASS
- M3.3a static PASS
- M3.3b static PASS
- native Bebbo build with `m68k-amigaos-gcc -m68000`
- AmigaOS loadseg()-compatible executable

## Required runtime gates

- version reports `0.3.0-m3.3b`
- invalid confirmation is rejected before modification
- missing, short and long source ADFs are rejected
- no-media destination is rejected
- write-protected destination is rejected
- writable disposable destination passes M3.3a preflight
- successful restore reports 1760 sectors written and 1760 sectors verified
- every sector is read back immediately after write
- post-restore `verify-adf` reports 1760 identical sectors
- a new disk image made after restore is byte-identical to the source ADF
- media-change behavior is controlled and documented
- M1, M2, M3.1, M3.2 and M3.3a regressions pass
- no crash, guru or hang

## Safety invariants

The only physical write primitive introduced by M3.3b is `ad_td_write_sector`, backed by one `CMD_WRITE` path in the trackdisk backend. There are no format paths. The M3.3a preflight remains read-only. Restore requires the existing exact `ERASE-DFn` confirmation and rechecks destination state after preflight. Destination `TD_CHANGENUM` is checked before and after each sector write/readback cycle.

A write is not considered successful until the same sector has been read back and compared byte-for-byte with its source ADF sector. Restore stops on the first source-read, destination-write, readback, verification, status or media-change error.

Final status may be changed to `M3.3b RUNTIME QUALIFICATION PASS` only after visible FS-UAE runtime evidence is recorded here.
