#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COPY = (ROOT / "src/operations/copy_disk.c").read_text(encoding="utf-8")
COPY_H = (ROOT / "src/operations/copy_disk.h").read_text(encoding="utf-8")
TRACKDISK = (ROOT / "src/io/trackdisk/trackdisk.c").read_text(encoding="utf-8")
MAIN = (ROOT / "src/main.c").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")
VERSION = (ROOT / "src/core/ad_version.h").read_text(encoding="utf-8")

for token in [
    "ad_copy_disk",
    "AD_COPY_DISK_ERR_CONFIRMATION",
    "AD_COPY_DISK_ERR_SOURCE_CHANGED",
    "AD_COPY_DISK_ERR_DEST_CHANGED",
    "AD_COPY_DISK_ERR_DEST_WRITE",
    "AD_COPY_DISK_ERR_VERIFY",
    "sectors_read",
    "sectors_written",
    "sectors_verified",
]:
    assert token in COPY_H, f"missing M3.4 copy API token: {token}"

for token in [
    "source_unit == destination_unit",
    'sprintf(expected_confirmation, "ERASE-DF%u"',
    "ad_td_get_status(&source",
    "ad_td_get_status(&destination",
    "destination_status.write_protected",
    "ad_td_get_change_number",
    "ad_td_read_sector(&source",
    "ad_td_write_sector(&destination",
    "ad_td_read_sector(&destination",
    "memcmp(",
    "AD_TD_CYLINDERS",
    "AD_TD_HEADS",
    "AD_TD_SECTORS_PER_TRACK",
]:
    assert token in COPY, f"missing M3.4 copy safety token: {token}"

assert COPY.count("ad_td_write_sector(&destination") == 1, \
    "M3.4 must use one call site to the qualified sector-write primitive"
assert COPY.count("ad_td_get_change_number") == 4, \
    "M3.4 must read initial source/destination change numbers and both again in the shared change guard"
assert COPY.count("ad_copy_check_changes(") >= 4, \
    "M3.4 must check both media before/after destructive sector cycles"

for token in ["CMD_WRITE", "CMD_UPDATE", "CMD_CLEAR", "TD_FORMAT", "ETD_WRITE", "ETD_FORMAT"]:
    assert token not in COPY, f"M3.4 must not bypass the qualified trackdisk write primitive: {token}"

assert TRACKDISK.count("CMD_WRITE") == 1, \
    "physical write surface must remain exactly one CMD_WRITE in trackdisk backend"
assert TRACKDISK.count("CMD_UPDATE") == 1
assert TRACKDISK.count("CMD_CLEAR") == 1

assert "copy-disk" in MAIN, "M3.4 CLI command missing"
assert "source and destination units must differ" in MAIN
assert "WARNING: this command overwrites the destination disk." in MAIN
assert "src/operations/copy_disk.c" in MAKEFILE
assert "tools/check_m3_4.py" in MAKEFILE
assert 'AMIDISK_VERSION "0.3.0-m3.4"' in VERSION
assert 512 * 11 * 2 * 80 == 901120

print("M3.4 static checks: PASS")
