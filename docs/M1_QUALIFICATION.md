# M1 qualification

## Scope

M1 introduces the first real Amiga hardware backend. It is deliberately read-only.

## Implemented

- `trackdisk.device` units 0 through 3
- safe message-port and I/O-request lifecycle
- open/close device handling
- media presence/change-state query with `TD_CHANGESTATE`
- write-protect query with `TD_PROTSTATUS`
- standard Amiga DD geometry: 80 cylinders, 2 heads, 11 sectors, 512 bytes
- one-sector reads through `CMD_READ`
- deterministic AmiDisk result codes
- diagnostic `probe` command
- diagnostic `read-sector` command
- range validation before I/O
- explicit no-media result
- CI static guard preventing write/format commands in M1

## Safety boundary

M1 contains no disk write or format operation. The following command families are explicitly forbidden by the M1 static qualification check:

- `CMD_WRITE`
- `TD_FORMAT`
- `ETD_WRITE`
- `ETD_FORMAT`

## Qualification levels

### Host/static qualification

Required for merge to `main`:

- repository hygiene passes
- M1 API/geometry checks pass
- required Exec/trackdisk calls are present
- read-only guard passes
- native build includes the trackdisk backend

### Native build qualification

Must be verified with Bebbo GCC using the repository Makefile and `-m68000`.

### Runtime qualification

Runtime qualification is required before M1 is declared fully complete. Use a visible Amiga/FS-UAE session and verify at minimum:

1. `AmiDisk --version`
2. `AmiDisk probe 0` with media inserted
3. `AmiDisk probe 0` with media removed
4. write-protected media reports correctly
5. `AmiDisk read-sector 0 0 0 0` succeeds on a known-good standard Amiga disk
6. out-of-range cylinder/head/sector is rejected without device I/O
7. DF1-DF3 failure/absence is handled without crash

No write test is part of M1.
