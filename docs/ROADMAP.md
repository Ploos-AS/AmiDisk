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

Controlled media workflows are introduced incrementally behind explicit qualification gates.

### M3.1 - Disk to ADF imaging

GREEN. DFx to newly-created standard ADF. Physical media remains read-only. Existing destination files are never overwritten; partial output is removed on failure; source disk changes are guarded with `TD_CHANGENUM`. Visible FS-UAE runtime qualification complete.

### M3.2 - Verify

GREEN. Physical disk and standard ADF are compared sector-by-sector with deterministic mismatch location, no-media handling and media-change protection. Visible FS-UAE runtime qualification complete. Strictly read-only.

### M3.3a - Restore safety preflight

GREEN. Source ADF, destination media state, write protection, destination readability, exact confirmation and media-change stability are validated before restore. Visible FS-UAE runtime qualification complete; the preflight itself remains permanently read-only.

### M3.3b - Restore engine

GREEN. Standard ADF to writable DFx restore through one guarded sector-write primitive. Visible FS-UAE runtime qualification complete. Every physical write is flushed with `CMD_UPDATE`, the track cache is invalidated with `CMD_CLEAR`, and the sector is immediately read back and compared. Exact confirmation, write protection, no-media handling and destination media-change guards are qualified.

### M3.4 - Disk to disk copy

Implemented, runtime qualification pending. Copies a standard Amiga DD disk from one physical DFx unit to a different DFy unit. Destination overwrite requires exact `ERASE-DFy` confirmation. Source and destination media-change numbers are guarded throughout the operation, the M3.3b qualified sector-write primitive is reused unchanged, and every destination sector is immediately read back and compared before proceeding.

### Later M3 work

Disk-to-RAM-to-disk, per-track state and additional copy workflows.

## M4 - Recovery

Retries, partial images, bad-sector/track maps, resumable imaging and evidence-preserving reconstruction.

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
