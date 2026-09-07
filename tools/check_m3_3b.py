#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRACKDISK = (ROOT / "src/io/trackdisk/trackdisk.c").read_text(encoding="utf-8")
TRACKDISK_H = (ROOT / "src/io/trackdisk/trackdisk.h").read_text(encoding="utf-8")
RESTORE = (ROOT / "src/operations/restore_adf.c").read_text(encoding="utf-8")
RESTORE_H = (ROOT / "src/operations/restore_adf.h").read_text(encoding="utf-8")
PREFLIGHT = (ROOT / "src/operations/restore_preflight.c").read_text(encoding="utf-8")
MAIN = (ROOT / "src/main.c").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")

for token in ["ad_td_write_sector", "AD_TD_ERR_WRITE_PROTECTED"]:
    assert token in TRACKDISK_H, f"missing M3.3b trackdisk token: {token}"

assert TRACKDISK.count("CMD_WRITE") == 1, "M3.3b permits exactly one physical write primitive"
for token in ["TD_FORMAT", "ETD_WRITE", "ETD_FORMAT"]:
    assert token not in TRACKDISK + RESTORE + MAIN, f"forbidden M3.3b write/format path: {token}"

for token in [
    "ad_restore_adf_to_disk",
    "AD_RESTORE_ERR_MEDIA_CHANGED",
    "AD_RESTORE_ERR_DEST_WRITE",
    "AD_RESTORE_ERR_VERIFY",
]:
    assert token in RESTORE_H, f"missing M3.3b restore API token: {token}"

for token in [
    "ad_restore_preflight",
    "ad_adf_read_sector",
    "ad_td_write_sector",
    "ad_td_read_sector",
    "memcmp(",
    "ad_td_get_change_number",
    "sectors_written",
    "sectors_verified",
]:
    assert token in RESTORE, f"missing M3.3b restore safety token: {token}"

assert "ad_td_write_sector" not in PREFLIGHT, "M3.3a preflight must remain read-only"
assert "restore-adf" in MAIN, "M3.3b CLI command missing"
assert "WARNING: this command overwrites the destination disk." in MAIN
assert "src/operations/restore_adf.c" in MAKEFILE
assert "tools/check_m3_3b.py" in MAKEFILE
assert 512 * 11 * 2 * 80 == 901120
print("M3.3b static checks: PASS")
