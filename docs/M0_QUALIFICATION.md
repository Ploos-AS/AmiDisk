# M0 qualification

M0 is a foundation milestone and does not qualify physical floppy I/O.

## Foundation gates

- [x] Product scope documented
- [x] MIT license present
- [x] Motorola 68000 target encoded in build flags
- [x] AmigaOS 2.04+ baseline documented
- [x] M0 executable contains no disk-write implementation
- [x] Architecture separates frontends, operations and I/O backends
- [x] ADF is the first image format
- [x] Gotek is supported through standard-drive behavior where applicable
- [x] Greaseweazle has a distinct future flux-backend boundary
- [x] Host repository checks and GitHub CI are present

Native build and runtime qualification becomes mandatory in M1 when AmigaOS device access is introduced.
