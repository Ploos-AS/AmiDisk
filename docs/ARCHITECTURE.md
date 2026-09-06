# AmiDisk architecture

## Principles

1. Native Amiga first: Motorola 68000 and AmigaOS 2.04+ are the reference baseline.
2. Low-level before filesystem: copying and imaging must work without OFS/FFS knowledge.
3. Replaceable backends: physical floppy, image and future flux I/O share core operations without pretending they have identical semantics.
4. Read-only first: new backends are qualified before destructive write paths are enabled.
5. Recovery preserves evidence: failed and partial reads remain observable.
6. UI is not the engine: GUI, CLI and ARexx frontends use the same core services.

## Layers

```text
GUI / CLI / ARexx
        |
Copy / Image / Recover
        |
Analysis / Verify
        |
Block / Track abstraction
        |
Physical | Image | Flux
DF0-DF3  | ADF   | Greaseweazle later
```

## Physical floppy

M1 introduces a read-only `trackdisk.device` backend for standard Amiga floppy units. It will handle safe open/close, media changes, write-protect state, reads and explicit error mapping. Write and format operations come later.

A Gotek behaving as a normal Amiga floppy drive is expected to work through this backend; AmiDisk does not need to special-case it for basic disk operations.

## Images

ADF is the first image format and baseline for standard AmigaDOS disk imaging. Extended ADF, DMS and raw/custom-track representations are later work.

## Flux

Greaseweazle is intentionally a distinct future backend. Flux capture can preserve timing, multiple revolutions, unstable regions and undecoded data that cannot be represented faithfully as ordinary sectors.

## Safety

Destructive operations must be explicit. A read failure must never silently become fabricated good data. Repair, filesystem modification, bootblock modification, format and overwrite are separate operations from observation and imaging.
