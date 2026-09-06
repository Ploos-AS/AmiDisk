# AmiDisk roadmap

## M0 - Foundation

- scope and invariants
- MIT license
- Motorola 68000 / AmigaOS 2.04+ baseline
- Bebbo GCC build skeleton
- repository checks and CI
- architecture and backend boundaries

## M1 - trackdisk read engine

Read-only DF0-DF3 backend: open/close, media change, write protection, standard reads, deterministic errors and diagnostic CLI. Native qualification required.

## M2 - Copy and verify

DFx-to-DFy, disk-to-RAM-to-disk, verify-after-write, per-track state and explicit destructive-operation confirmation.

## M3 - ADF engine

Create, restore and verify standard ADF images with robust short-I/O handling.

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
