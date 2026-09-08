#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RECOVERY = (ROOT / "src/operations/recovery_read.c").read_text(encoding="utf-8")
RECOVERY_H = (ROOT / "src/operations/recovery_read.h").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")
VERSION = (ROOT / "src/core/ad_version.h").read_text(encoding="utf-8")

for token in [
    "AD_RECOVERY_MAX_ATTEMPTS",
    "AdRecoveryReadResult",
    "AD_RECOVERY_READ_ERR_MEDIA_CHANGED",
    "AD_RECOVERY_READ_ERR_EXHAUSTED",
    "AdRecoverySectorRecord",
    "attempts",
    "start_change_number",
    "end_change_number",
    "last_source_result",
    "ad_recovery_read_sector",
]:
    assert token in RECOVERY_H, f"missing M4.1 recovery API token: {token}"

for token in [
    "max_attempts == 0UL",
    "max_attempts > AD_RECOVERY_MAX_ATTEMPTS",
    "ad_td_get_status(&disk",
    "status.media_present",
    "ad_td_get_change_number(&disk",
    "for (attempt = 1UL; attempt <= max_attempts; ++attempt)",
    "ad_td_read_sector(&disk",
    "AD_RECOVERY_READ_ERR_EXHAUSTED",
    "AD_RECOVERY_READ_ERR_MEDIA_CHANGED",
]:
    assert token in RECOVERY, f"missing M4.1 recovery safety token: {token}"

assert RECOVERY.count("ad_td_read_sector(&disk") == 1, \
    "M4.1 retries must use one shared read call site"
assert RECOVERY.count("ad_td_get_change_number(&disk") >= 3, \
    "M4.1 must guard media identity before/during/after recovery reads"

for token in ["CMD_WRITE", "CMD_UPDATE", "CMD_CLEAR", "TD_FORMAT", "ETD_WRITE", "ETD_FORMAT", "ad_td_write_sector"]:
    assert token not in RECOVERY, f"M4.1 recovery foundation must remain source read-only: {token}"

assert "src/operations/recovery_read.c" in MAKEFILE, "M4.1 source missing from build"
assert "tools/check_m4_1.py" in MAKEFILE, "M4.1 static gate missing from make check"
assert 'AMIDISK_VERSION "0.4.0-m4.1"' in VERSION, "M4.1 version missing"

print("M4.1 static checks: PASS")
