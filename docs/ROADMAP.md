# AmiDisk roadmap

## M0 - Foundation

- scope and invariants
- MIT license
- Motorola 68000 / AmigaOS 2.04+ baseline
- Bebbo GCC build skeleton
- repository checks and CI
- architecture and backend boundaries

## M1 - trackdisk read engine

Read-only DF0-DF3 backend: open/close, media change, write protection, standard reads, deterministic errors and diagnostic CLI. Native runtime qualification complete.

## M2 - Standard ADF read engine

Read-only standard 880 KiB ADF backend: exact geometry/size validation, deterministic errors, CHS sector reads and diagnostic CLI. Native runtime qualification complete.

## M3 - Copy, image and verify

GREEN. Controlled media workflows were introduced incrementally behind explicit qualification gates. M3.1 through M3.4 are all runtime-qualified in visible FS-UAE on the Motorola 68000 / AmigaOS 2.04+ baseline.

### M3.1 - Disk to ADF imaging

GREEN. DFx to newly-created standard ADF. Physical media remains read-only. Existing destination files are never overwritten; partial output is removed on failure; source disk changes are guarded with `TD_CHANGENUM`. Visible FS-UAE runtime qualification complete.

### M3.2 - Verify

GREEN. Physical disk and standard ADF are compared sector-by-sector with deterministic mismatch location, no-media handling and media-change protection. Visible FS-UAE runtime qualification complete. Strictly read-only.

### M3.3a - Restore safety preflight

GREEN. Source ADF, destination media state, write protection, destination readability, exact confirmation and media-change stability are validated before restore. Visible FS-UAE runtime qualification complete; the preflight itself remains permanently read-only.

### M3.3b - Restore engine

GREEN. Standard ADF to writable DFx restore through one guarded sector-write primitive. Visible FS-UAE runtime qualification complete. Every physical write is flushed with `CMD_UPDATE`, the track cache is invalidated with `CMD_CLEAR`, and the sector is immediately read back and compared. Exact confirmation, write protection, no-media handling and destination media-change guards are qualified.

### M3.4 - Disk to disk copy

GREEN. Standard Amiga DD disks can be copied directly from one physical DFx unit to a different DFy unit. Visible FS-UAE runtime qualification completed with 1760 sectors read, 1760 sectors written, 1760 sectors read-back verified and a byte-identical destination image while the source remained unchanged. Destination overwrite requires exact `ERASE-DFy` confirmation. Source and destination media-change numbers are guarded throughout the operation, and the M3.3b qualified `CMD_WRITE` -> `CMD_UPDATE` -> `CMD_CLEAR` sector-write primitive is reused unchanged.

### Later M3 work

Disk-to-RAM-to-disk, per-track state and additional copy workflows may be added later where they support recovery, GUI or preservation work. They are not required for M3 GREEN status.

## M4 - Recovery

Recovery remains source-read-only. The recovery path must never silently present substituted bytes as successfully recovered data; unreadable areas need explicit state/evidence.

### M4.1 - Recovery read foundation

GREEN. Adds bounded per-sector retry policy (1-16 attempts), deterministic retry accounting, real `TD_CHANGENUM` guards before/during/after reads, no-media handling and an explicit sector recovery record carrying CHS, attempts and last trackdisk result. No physical write primitive is reachable from this module. Visible runtime qualification passed on the Motorola 68000 / AmigaOS 2.04+ baseline.

### M4.2 - Partial imaging and bad-sector map

Implemented; host/native and runtime qualification pending. Builds a complete 901120-byte ADF-shaped recovery artifact together with an authoritative tab-separated sector map covering all 1760 sectors. Each sector is explicitly `GOOD`, `BAD` or `UNREAD`: GOOD contains recovered source bytes, while BAD/UNREAD positions contain zero placeholders that are never represented as recovered data. BAD means the configured M4.1 retry budget was exhausted; UNREAD means acquisition stopped trusting the source after a source/media-identity failure. Image/map outputs use exclusive creation, source identity is guarded across the whole acquisition with `TD_CHANGENUM`, and the source remains physically read-only.

### M4.3 - Resumable recovery

Resume from prior recovery evidence without re-reading already trusted sectors unless requested. Preserve acquisition provenance and source-media identity/change information.

### M4.4 - Reconstruction and multi-pass policy

Add controlled additional passes, track-oriented retry ordering and evidence-preserving reconstruction rules. Never modify the source disk.

## M5 - GUI

X-Copy-inspired track overview, source/destination selection, keyboard-first operation and per-track state.

## M6 - Analysis

Bootblocks, OFS/FFS recognition, filesystem observations, hashes and metadata; read-only first.

## M7 - Security

Optional virus/signature analysis and clean-copy workflows, kept separate from core imaging.

## M8 - Automation

Stable CLI, ARexx port and repeatable imaging/verification workflows.

## M9 - Preservation

Custom tracks, Extended ADF/raw representations, Greaseweazle integration, flux capture, multi-read comparison and preservation metadata.
