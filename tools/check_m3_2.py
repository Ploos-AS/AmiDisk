#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERIFY = (ROOT / "src/operations/verify_adf.c").read_text(encoding="utf-8")
VERIFY_H = (ROOT / "src/operations/verify_adf.h").read_text(encoding="utf-8")
MAIN = (ROOT / "src/main.c").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")
TRACKDISK = (ROOT / "src/io/trackdisk/trackdisk.c").read_text(encoding="utf-8")
ADF = (ROOT / "src/io/adf/adf.c").read_text(encoding="utf-8")

for token in [
    "ad_verify_disk_against_adf",
    "AD_VERIFY_MISMATCH",
    "AD_VERIFY_ERR_NO_MEDIA",
    "AD_VERIFY_ERR_MEDIA_CHANGED",
    "first_absolute_offset",
    "mismatch_sectors",
]:
    assert token in VERIFY_H, f"missing M3.2 verify API token: {token}"

for token in [
    "ad_td_read_sector",
    "ad_adf_read_sector",
    "ad_td_get_change_number",
    "memcmp(",
    "first_absolute_offset",
    "AD_TD_CYLINDERS",
    "AD_TD_HEADS",
    "AD_TD_SECTORS_PER_TRACK",
]:
    assert token in VERIFY, f"missing M3.2 verify implementation token: {token}"

assert "verify-adf" in MAIN, "M3.2 CLI command missing"
assert "src/operations/verify_adf.c" in MAKEFILE, "verify source missing from native build"
assert "tools/check_m3_2.py" in MAKEFILE, "M3.2 static check missing from make check"

for token in [
    "fwrite(", "remove(", "rename(", "unlink(", "O_WRONLY", "O_RDWR",
    'fopen(path, "wb")', 'fopen(path, "w")', "CMD_WRITE", "TD_FORMAT",
    "ETD_WRITE", "ETD_FORMAT",
]:
    assert token not in VERIFY, f"M3.2 verify must remain read-only: {token}"

all_physical = "\n".join([TRACKDISK, VERIFY, MAIN])
for token in ["CMD_WRITE", "TD_FORMAT", "ETD_WRITE", "ETD_FORMAT"]:
    assert token not in all_physical, f"physical write/format forbidden before M3.3: {token}"

assert 'fopen(path, "rb")' in ADF, "ADF backend must remain read-only for verify"
assert 512 * 11 * 2 * 80 == 901120
print("M3.2 static checks: PASS")
