#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRACKDISK = (ROOT / "src/io/trackdisk/trackdisk.c").read_text(encoding="utf-8")
TRACKDISK_H = (ROOT / "src/io/trackdisk/trackdisk.h").read_text(encoding="utf-8")
IMAGING = (ROOT / "src/operations/image_adf.c").read_text(encoding="utf-8")
IMAGING_H = (ROOT / "src/operations/image_adf.h").read_text(encoding="utf-8")
MAIN = (ROOT / "src/main.c").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")

for token in [
    "ad_image_disk_to_adf",
    "AD_IMAGE_ERR_DEST_EXISTS",
    "AD_IMAGE_ERR_SOURCE_READ",
    "AD_IMAGE_ERR_MEDIA_CHANGED",
]:
    assert token in IMAGING_H, f"missing M3.1 imaging API token: {token}"

for token in [
    'fopen(path, "rb")',
    "O_EXCL",
    "fdopen(",
    "fwrite(",
    "remove(path)",
    "ad_td_read_sector",
    "ad_td_get_change_number",
    "AD_TD_CYLINDERS",
    "AD_TD_HEADS",
    "AD_TD_SECTORS_PER_TRACK",
]:
    assert token in IMAGING, f"missing M3.1 imaging implementation token: {token}"

assert "TD_CHANGENUM" in TRACKDISK, "M3.1 requires trackdisk change-number guard"
assert "ad_td_get_change_number" in TRACKDISK_H, "change-number API missing"
assert "image-adf" in MAIN, "M3.1 CLI command missing"
assert "qualify-media-change" in MAIN, "M3.1a interactive qualifier missing"
assert "Eject or swap the disk in FS-UAE now" in MAIN, "M3.1a visible-runtime instruction missing"
assert "after_change == before_change" in MAIN, "M3.1a change-number assertion missing"
assert "src/operations/image_adf.c" in MAKEFILE, "imaging source missing from native build"
assert "tools/check_m3_1.py" in MAKEFILE, "M3.1 static check missing from make check"

# M3.1 itself must remain non-destructive to physical media. Later milestones
# may legitimately extend the shared trackdisk backend and CLI with restore
# support, so scope this prohibition to the M3.1 imaging implementation.
for token in ["CMD_WRITE", "TD_FORMAT", "ETD_WRITE", "ETD_FORMAT"]:
    assert token not in IMAGING, f"M3.1 imaging path must not use physical write/format: {token}"

assert "AD_TD_DISK_BYTES" in TRACKDISK_H
assert 512 * 11 * 2 * 80 == 901120
print("M3.1 static checks: PASS")
