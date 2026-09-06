# AmiDisk

AmiDisk is an open-source disk imaging, copying, recovery and preservation utility for classic Amiga systems.

Inspired by classic track-oriented tools such as X-Copy, AmiDisk aims for a modern architecture covering physical floppy drives, disk images, recovery, analysis and later preservation workflows.

## M0 baseline

- native Amiga program
- Motorola 68000 baseline
- AmigaOS 2.04+ baseline
- Bebbo GCC / m68k-amigaos toolchain
- MIT license
- `trackdisk.device` as first physical-drive backend
- ADF as first image format
- Gotek compatibility through the normal floppy path
- Greaseweazle reserved for a later flux backend
- low-level copying and imaging independent of filesystem support

## Planned capabilities

Copy, verify, format, ADF imaging/restoration, recovery, track and sector inspection, OFS/FFS analysis, bootblock inspection, hashing, optional virus analysis, CLI/ARexx automation and preservation-oriented formats.

See `docs/ARCHITECTURE.md` and `docs/ROADMAP.md`.

## Build

```sh
make
```

## Checks

```sh
make check
```

## License

MIT. See `LICENSE`.
