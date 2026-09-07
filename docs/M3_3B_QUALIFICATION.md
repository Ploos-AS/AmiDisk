# M3.3b Restore Engine runtime qualification

Status: M3.3b RUNTIME QUALIFICATION PASS.

## Environment and revisions

Qualification date: 2026-09-07. Only visible FS-UAE 3.2.35 was used, with
A500 / Motorola 68000, no FPU/MMU/JIT, Kickstart 2.04 / 37.175 and
Workbench 2.04 / 37.67. Standard Amiga DD geometry: 80 cylinders, 2 heads,
11 sectors/track, 512-byte sectors, 1760 sectors, 901120 bytes.

Starting HEAD and starting origin/main:
`01f96acfd8b764c22518560da40a8e26f4a3adc6`. Fetch, checkout main and ff-only
pull succeeded; initial worktree was clean and divergence was `0 0`.
The host evidence file itself appears as untracked in the subsequently recorded
status because it was created to capture the gate. All seven initial static
gates and the native Bebbo build passed. [Initial host gate](evidence/m3.3b/initial-host.txt).

Runtime audit found and fixed a physical readback defect. The final production
revision is `4b57f8643408cb923de55de6e1c3fbf4e4a120cb`, committed and pushed
to main before rebuilding and repeating runtime qualification. The final binary
is 28688 bytes, SHA-256
`acc2e0cd30715113432fa301a8cd4f8d3bea72861e2df55ac6591e77e87bcb64`.
Its version remains `AmiDisk 0.3.0-m3.3b`,
`Motorola 68000 / AmigaOS 2.04+`.

Bebbo compiler: `m68k-amigaos-gcc (GCC) 6.5.0b 20260807212032`.
Build flags include `-m68000 -noixemul`; `file AmiDisk` reports
`AmiDisk: AmigaOS loadseg()ble executable/binary`.

## Physical readback correction

The original primitive issued a sector write followed by a restore-level read.
According to the [trackdisk autodocs](https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node05B3.html),
write and read can both use the track buffer. That sequence did not establish
physical per-sector readback, even when memcmp passed.

The dedicated primitive now performs a 512-byte write, flushes with CMD_UPDATE,
then invalidates the track buffer with CMD_CLEAR. It returns immediately on
write or flush failure; it never clears after a failed flush. Restore then reads
the same sector from media, compares all 512 bytes, and returns before advancing
on read failure or mismatch. Static gates were strengthened to require this
order and error returns; no safety gate was weakened and no test backdoor was
added. [Physical-write audit](evidence/m3.3b/safety-audit.txt).

The preliminary original-binary run reported RC 0 and 1760 written/verified,
but is not used as final qualification proof. Its following verify was ended
when that exploratory emulator session was closed to retest the correction.
Preliminary evidence is isolated under [original](evidence/m3.3b/original/).

## Disposable fixtures and execution

The final session uses `/tmp/amidisk-m33b-final/Workbench2.0`, extracted from a
local known-good Workbench 2.04 ADF. Work: is assigned to SYS:. DF0 is exclusively
`/tmp/amidisk-m33b-final/destination.adf`, a new disposable copy of the local
Extras 2.04 ADF. No original or physical floppy device was attached or written.
The earlier exploratory session used the separate `/tmp/amidisk-m33b` tree.
FS-UAE stores disk writes in its disposable SDF overlay; a hash of the backing
ADF alone is therefore not treated as proof of guest-visible post-write data.
The before/after images are produced by AmiDisk through trackdisk.

The final protected session runs first, with emulator write protection enabled.
It is closed normally and the same pristine destination is started writable.
All remaining operations use that visible writable session and GUI disk changes.
Startup/script execution is visible; there is no headless runtime.
Each guest script captures `$RC` immediately following each command, before
Type/Echo can replace it. stdout is retained in .out files; stderr diagnostics
remain on the visible console and are captured in screenshots.

Initial attempts to use global desktop input reached the chat instead of the
emulator. The user manually started the first baseline. Subsequent automation
used X11 key events addressed directly to the visible FS-UAE window. An initial
typo and a harmless echo used to validate input are not qualification results.
Menu navigation mistakes were corrected visually before selecting media.

## Baseline and rejection gates (final binary)

Source `Work:restore-source.adf`: 901120 bytes, standard geometry confirmed by
adf-info, RC 0. SHA-256:
`9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef`.
Short and long fixtures are exactly 901119 and 901121 bytes; the missing path
is absent. The source is a separate file from destination.

Writable baseline probe: `media=present write-protected=no`, RC 0.
BEFORE imaging: RC 0, 1760 sectors, 901120 bytes. SHA-256:
`3da86648e602f4e0916663d2b4893011423837b2f2b6885e421310338412c02a`.
BEFORE differs from source. Pre-restore verify returns RC 3. The preliminary
visible diagnostic identifies 1620 mismatching sectors, first C0/H0/S0 byte 4,
absolute offset 4, disk=00 and adf=e3; the final-binary retest again returns RC 3
on byte-identical baseline/source fixtures. This first-mismatch diagnostic is
also independently consistent with those complete host images.

| Gate | Runtime result | RC |
| --- | --- | --- |
| WRONG | confirmation rejected; zero sectors written | 2 |
| ERASE-DF1 for DF0 | confirmation rejected; zero sectors written | 2 |
| erase-df0 / ERASE-df0 | rejected | 2 each |
| XERASE-DF0 / ERASE-DF0X / ERASE-DF | rejected | 2 each |
| Missing source | source ADF rejected; written=0 verified=0 | 2 |
| Short source | source ADF rejected; written=0 verified=0 | 2 |
| Long source | source ADF rejected; written=0 verified=0 | 2 |
| Protected probe | media=present write-protected=yes | 0 |
| Protected restore, exact ERASE-DF0 | write-protected rejection; written=0 verified=0 | 2 |
| Ejected probe | media=absent | 0 |
| Ejected restore, exact ERASE-DF0 | no destination media; written=0 verified=0 | 2 |

The final-binary control image `reject-after.adf` completes with RC 0, 1760
sectors and 901120 bytes, and is byte-identical to BEFORE. This covers the
protected rejection and confirmation/invalid-source tests before any successful
restore. No-media has no destination to modify.

The separate `qualify-media-change 0` command held the device open across GUI
eject and reported present/change=0 followed by absent/change=1, RC 0. This
is a real TD_CHANGENUM runtime observation, repeated with the final binary.

## Full restore, proofs and regressions

The corrected final binary completed the central writable test with RC 0:
`sectors-written=1760 sectors-verified=1760 bytes=901120 change=2`.
The immediately following verify, without eject or swap, returned RC 0:
`identical sectors=1760`. AFTER imaging returned RC 0, 1760 sectors and
901120 bytes. Source and AFTER both hash to
`9023055f9fdc948f2f46fddd49776303cc9c1a997e86cbe927ec60d8c6ffe9ef` and
host `cmp` passes. BEFORE hashes to
`3da86648e602f4e0916663d2b4893011423837b2f2b6885e421310338412c02a` and
differs from both source and AFTER. The regression image is also 901120 bytes
and source-identical. [Final proof](evidence/m3.3b/final-proof.json).

Spot checks on source and restored DF0 matched for all three requested sectors:
C0/H0/S0 prefix `44 4f 53 00 e3 3d 0e 73 00 00 03 70 43 fa 00 3e`,
C10/H1/S5 prefix `00 00 00 08 00 00 00 e5 00 00 00 07 00 00 01 e8`, and
C79/H1/S10 prefix `00 00 00 08 00 00 06 dd 00 00 00 02 00 00 00 60`.
Regression commands all returned RC 0:
probe, read-sector, adf-info, adf-read-sector, verify-adf (1760 sectors),
image-adf (1760/901120), and restore-preflight (`NO WRITE PERFORMED`).

An explicit restore/eject timing attempt was made after the successful proof.
The restore result file still contained the prior successful RC 0 result. The
auxiliary X11 screenshot helper exited with host status 139 when the window
closed, before a controlled restore-specific media-change diagnostic could be
captured; no Amiga guru or guest crash was observed. This is not claimed as an
exact restore-specific media-change runtime PASS. The separate repeated
`qualify-media-change` test is the accepted real TD_CHANGENUM observation
(0 → 1, RC 0); restore-path change checks are statically verified.

No crash, guru or hang occurred during the qualification runs themselves;
the final timing experiment's termination is recorded above. Read-back mismatch
is statically verified after physical flush/invalidation because FS-UAE did not
provide a natural way to induce a bad sector without production backdoors.

## Final status

**M3.3b RUNTIME QUALIFICATION PASS.** Full restore completed on the corrected
binary with 1760 sectors written and read-back-verified, immediate verify was
identical, AFTER was byte-identical to source, BEFORE differed, write-protect
and no-media were runtime-observed, and all static/regression gates passed.
The restore-specific eject branch remains separately documented as not
runtime-observed.
