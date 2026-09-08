#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
IMAGE = (ROOT / "src/operations/recovery_image.c").read_text(encoding="utf-8")
IMAGE_H = (ROOT / "src/operations/recovery_image.h").read_text(encoding="utf-8")
MAIN = (ROOT / "src/main.c").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")
VERSION = (ROOT / "src/core/ad_version.h").read_text(encoding="utf-8")

for token in [
    "AD_RECOVERY_SECTOR_GOOD",
    "AD_RECOVERY_SECTOR_BAD",
    "AD_RECOVERY_SECTOR_UNREAD",
    "AD_RECOVERY_IMAGE_PARTIAL",
    "AdRecoveryImageReport",
    "good_sectors",
    "bad_sectors",
    "unread_sectors",
    "placeholder_sectors",
    "first_bad_valid",
    "first_unread_valid",
    "ad_recovery_image_disk",
]:
    assert token in IMAGE_H, f"missing M4.2 API token: {token}"

for token in [
    "O_EXCL",
    "AMIDISK_RECOVERY_MAP\\t1",
    "placeholder_byte\\t00",
    "GOOD",
    "BAD",
    "UNREAD",
    "ad_recovery_read_sector",
    "AD_RECOVERY_READ_ERR_EXHAUSTED",
    "ad_td_get_change_number(&monitor",
    "AD_RECOVERY_TOTAL_SECTORS",
    "AD_TD_DISK_BYTES",
    "ad_recovery_fill_unread",
]:
    assert token in IMAGE, f"missing M4.2 recovery/evidence token: {token}"

assert IMAGE.count("ad_recovery_read_sector(") == 1, \
    "M4.2 must reuse one M4.1 recovery-read call site"
assert IMAGE.count("ad_td_get_change_number(&monitor") >= 3, \
    "M4.2 must guard source identity across the full acquisition"

for token in ["CMD_WRITE", "CMD_UPDATE", "CMD_CLEAR", "TD_FORMAT", "ETD_WRITE", "ETD_FORMAT", "ad_td_write_sector"]:
    assert token not in IMAGE, f"M4.2 source path must remain physically read-only: {token}"

for token in [
    '#include "operations/recovery_image.h"',
    "recover-image",
    "command_recover_image",
    "ad_recovery_image_disk(unit, image_path, map_path, attempts",
    "recover-image OK:",
    "recover-image PARTIAL:",
    "consult map",
]:
    assert token in MAIN, f"missing M4.2 CLI token: {token}"

assert "src/operations/recovery_image.c" in MAKEFILE, "M4.2 source missing from build"
assert "tools/check_m4_2.py" in MAKEFILE, "M4.2 gate missing from make check"
assert 'AMIDISK_VERSION "0.4.0-m4.2"' in VERSION, "M4.2 version missing"
assert 512 * 11 * 2 * 80 == 901120

print("M4.2 static checks: PASS")
