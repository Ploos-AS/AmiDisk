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

## Status

**M3.3a IMPLEMENTED - RUNTIME QUALIFICATION PENDING.**

The next milestone must not start until M3.3a is GREEN.
