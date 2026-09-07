#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PREFLIGHT = (ROOT / "src/operations/restore_preflight.c").read_text(encoding="utf-8")
PREFLIGHT_H = (ROOT / "src/operations/restore_preflight.h").read_text(encoding="utf-8")
MAIN = (ROOT / "src/main.c").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")

for token in [
    "ad_restore_preflight",
    "AD_RESTORE_PREFLIGHT_ERR_CONFIRMATION",
    "AD_RESTORE_PREFLIGHT_ERR_WRITE_PROTECTED",
    "AD_RESTORE_PREFLIGHT_ERR_MEDIA_CHANGED",
]:
    assert token in PREFLIGHT_H, f"missing M3.3a preflight API token: {token}"

for token in [
    '"ERASE-DF%u"',
    "ad_adf_open",
    "ad_td_open",
    "ad_td_get_status",
    "write_protected",
    "ad_td_get_change_number",
    "ad_td_read_sector",
]:
    assert token in PREFLIGHT, f"missing M3.3a safety token: {token}"

assert "restore-preflight" in MAIN, "M3.3a CLI command missing"
assert "NO WRITE PERFORMED" in MAIN, "M3.3a no-write banner missing"
assert "src/operations/restore_preflight.c" in MAKEFILE, "preflight source missing from native build"
assert "tools/check_m3_3a.py" in MAKEFILE, "M3.3a static check missing from make check"

# M3.3a remains permanently read-only even after later milestones add writes elsewhere.
for token in ["CMD_WRITE", "TD_FORMAT", "ETD_WRITE", "ETD_FORMAT", "ad_td_write_sector"]:
    assert token not in PREFLIGHT, f"M3.3a preflight must remain read-only: {token}"

for token in ["fwrite(", 'fopen(path, "wb")']:
    assert token not in PREFLIGHT, f"M3.3a preflight must not write files: {token}"

print("M3.3a static checks: PASS")
