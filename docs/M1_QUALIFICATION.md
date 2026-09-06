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

## Native runtime qualification — 2026-09-06

Starting `main` and `origin/main`: `088e82e183da3a22833ace0b0b41106470286732`.
The starting worktree was clean. No work branch was created.

### Environment and build

- Host: Ubuntu 26.04 LTS, Linux 7.0.0-30-generic, x86-64.
- Compiler: `/opt/amiga/bin/m68k-amigaos-gcc`, Bebbo GCC
  `6.5.0b 20260807212032`.
- Compile flags: `-Isrc -Os -Wall -Wextra -Werror -m68000 -noixemul`;
  link: `m68k-amigaos-gcc -noixemul`.
- Visible, windowed FS-UAE 3.2.35; no headless emulator was used.
- A500, Motorola 68000, no FPU/MMU/JIT; emulator log confirms
  `CPU=68000, FPU=0, MMU=0, JIT=0`.
- Kickstart 2.04 revision 37.175; Workbench 2.04 revision 37.67.
- 1024 KiB chip RAM, 4096 KiB fast RAM, no slow RAM.
- One configured floppy drive (DF0); DF1–DF3 disabled.
- A temporary host directory mounted as `System:` contained the built
  `AmiDisk` and test scripts. Boot was from the Workbench ADF in DF0;
  commands ran in a visible Amiga Shell with current directory RAM:.
- Disk A: copy of `amiga-os-204-workbench.adf`, 901120 bytes, SHA-256
  `9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef`.
- Disk B: copy of `amiga-os-204-extras.adf`, 901120 bytes, SHA-256
  `3da86648e602f4e0916663d2b4893011423837b2f2b6885e421310338412c02a`.
- The write-protect test uses a separate visible session with
  `uae_floppy_write_protect = true`; all other emulated hardware is the same.
  This session boots `System:` at priority 10 and runs `--version` and
  `probe 0` from its startup-sequence in the visible boot console.

ROMs and operating-system images are local user-provided assets and are
not included in this repository. Test configuration and scripts are recorded
in [evidence/m1](evidence/m1/); paths refer to the temporary qualification setup.

### Failure found and fixed

The initial no-media test failed: both commands returned
`trackdisk.device I/O error`. `ad_td_get_status()` queried `TD_PROTSTATUS`
after detecting absent media; that query failed with an empty drive on this
runtime. Commit `513fc7f5def5617af8e0877ab137936d1946bcaa` returns the absent
status before querying protection. `write_protected` is initialized to false
when there is no medium; it is only meaningful when media is present.

M0 checks, M1 checks and the clean native build were rerun after the fix.
The fixed executable was copied into the running guest and all required
runtime cases were repeated. Its SHA-256 is
`14fb77636eb3c980e980034e9cd4454ca2e59ff17e1289eebb8d61ec0b704847`.

### Observed results on the fixed executable

| Test | Result | Observed output / evidence |
| --- | --- | --- |
| `AmiDisk --version` | PASS | `AmiDisk 0.1.0-m1`, `Motorola 68000 / AmigaOS 2.04+`; [screen](evidence/m1/present-fixed.png) |
| `AmiDisk probe 0`, disk A | PASS | `DF0: media=present write-protected=no`; [screen](evidence/m1/present-fixed.png) |
| `AmiDisk read-sector 0 0 0 0`, disk A | PASS | `DF0 C0 H0 S0 read OK`; [screen](evidence/m1/present-fixed.png) |
| `AmiDisk probe 0`, DF0 ejected | PASS | `DF0: media=absent write-protected=no`; [before/after fix](evidence/m1/absent-fixed.png) |
| `AmiDisk read-sector 0 0 0 0`, DF0 ejected | PASS | `DF0 C0 H0 S0: no media present`; [screen](evidence/m1/absent-fixed.png) |
| `AmiDisk read-sector 0 80 0 0` | PASS | `DF0 C80 H0 S0: sector address out of range`; [screen](evidence/m1/range-fixed.png) |
| `AmiDisk read-sector 0 0 2 0` | PASS | `DF0 C0 H2 S0: sector address out of range`; [screen](evidence/m1/range-fixed.png) |
| `AmiDisk read-sector 0 0 0 11` | PASS | `DF0 C0 H0 S11: sector address out of range`; [screen](evidence/m1/range-fixed.png) |
| `AmiDisk probe 1`, `probe 2`, `probe 3` | PASS | Each reports `could not open trackdisk.device unit`; [screen](evidence/m1/external-fixed.png) |
| `AmiDisk probe 0`, write-protected disk A | PASS | `DF0: media=present write-protected=yes`; [screen](evidence/m1/wp.png) |
| Disk A → eject → disk B, no emulator restart | PASS | A present/read OK, [absent](evidence/m1/change-ejected.png), then B present/read OK with different bytes; [screen](evidence/m1/change-fixed.png) |

First 16 bytes from disk A:

```text
44 4f 53 00 e3 3d 0e 73 00 00 03 70 43 fa 00 3e
```

First 16 bytes from disk B:

```text
44 4f 53 00 00 00 00 00 00 00 00 00 00 00 00 00
```

Both match the corresponding host ADF bytes. Every completed test returned
to the Shell without an observed crash, guru or hang. Early keyboard-layout
and menu-navigation mistakes were corrected and affected commands repeated;
those attempts were not counted as passing tests.

### Safety and limits

The only commands issued by the trackdisk backend remain `TD_CHANGESTATE`,
`TD_PROTSTATUS` and `CMD_READ`. No `CMD_WRITE`, `TD_FORMAT`, `ETD_WRITE` or
`ETD_FORMAT` was introduced. No write/format test was performed. Both test
ADF hashes remained unchanged.

Range rejection was observed in the guest. Source inspection confirms that
range validation returns before any `DoIO` path, including status queries;
the CLI does open/close the device before this validation. This qualification
did not instrument the emulator's disk bus or count I/O requests.

This qualifies the visible FS-UAE 68000/Kickstart 2.04 environment described
above, not physical drives or every later AmigaOS release. The disk-change
test used successive CLI invocations in the same uninterrupted emulator
session, consistent with AmiDisk's one-command-per-process interface.

The initial unprefixed `floppy_write_protect` setting did not enable guest
write protection and was not counted as a passing WP test. The corrected
setting uses the `uae_` prefix required for low-level UAE options
([FS-UAE documentation](https://fs-uae.net/docs/options/)).


### Final host qualification

After completion of runtime testing, `make clean`, `make check`, `make`
and `file AmiDisk` were repeated:

- M0 repository checks: PASS.
- M1 static checks: PASS.
- Native Bebbo GCC build with `-m68000`: PASS.
- `file AmiDisk`: `AmiDisk: AmigaOS loadseg()ble executable/binary`.
- Rebuilt binary SHA-256 matches the fixed executable tested in FS-UAE.

Before the documentation commit, `main` was one commit ahead of
`origin/main` (the no-media fix), with only qualification documentation and
evidence pending. The final documentation commit is the commit containing
this report; its hash is intentionally not embedded into its own contents.

**M1 RUNTIME QUALIFICATION PASS** for the environment and scope above.
