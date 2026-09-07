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

Controlled write paths introduced incrementally after M2 qualification.

### M3.1 - Disk to ADF imaging

GREEN. DFx to newly-created standard ADF. Physical media remains read-only. Existing destination files are never overwritten; partial output is removed on failure; source disk changes are guarded with `TD_CHANGENUM`. Visible FS-UAE runtime qualification complete.

### M3.2 - Verify

Implemented, qualification pending. Compare physical disk and standard ADF sector-by-sector, count mismatching sectors, report the first differing CHS/byte/absolute offset and guard against source-media changes. Strictly read-only.

### M3.3 - ADF to disk restore

First physical-media write path, with write-protect handling, explicit destructive confirmation and post-write verification.

### Later M3 work

DFx-to-DFy copy, disk-to-RAM-to-disk, per-track state and additional copy workflows.

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
