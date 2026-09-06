#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "src/io/adf/adf.h").read_text(encoding="utf-8")
SOURCE = (ROOT / "src/io/adf/adf.c").read_text(encoding="utf-8")
MAIN = (ROOT / "src/main.c").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")

required_header = [
    "AD_ADF_SECTOR_SIZE 512UL",
    "AD_ADF_SECTORS_PER_TRACK 11UL",
    "AD_ADF_HEADS 2UL",
    "AD_ADF_CYLINDERS 80UL",
    "ad_adf_open",
    "ad_adf_close",
    "ad_adf_read_sector",
]
required_source = [
    'fopen(path, "rb")',
    "AD_ADF_DISK_BYTES",
    "fseek",
    "ftell",
    "fread",
    "AD_ADF_ERR_SIZE",
    "AD_ADF_ERR_RANGE",
    "AD_ADF_ERR_READ",
]
required_main = ["adf-info", "adf-read-sector"]

for token in required_header:
    assert token in HEADER, f"missing M2 header token: {token}"
for token in required_source:
    assert token in SOURCE, f"missing M2 source token: {token}"
for token in required_main:
    assert token in MAIN, f"missing M2 CLI token: {token}"
assert "src/io/adf/adf.c" in MAKEFILE, "ADF backend missing from native build"

forbidden = ["fwrite(", "remove(", "rename(", 'fopen(path, "wb")', 'fopen(path, "w+b")']
for token in forbidden:
    assert token not in SOURCE, f"write/destructive operation forbidden in M2 ADF backend: {token}"

assert 512 * 11 * 2 * 80 == 901120
print("M2 static checks: PASS")
