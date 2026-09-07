#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "src/io/trackdisk/trackdisk.h").read_text(encoding="utf-8")
source = (ROOT / "src/io/trackdisk/trackdisk.c").read_text(encoding="utf-8")
main = (ROOT / "src/main.c").read_text(encoding="utf-8")
makefile = (ROOT / "Makefile").read_text(encoding="utf-8")

errors = []

required_header_tokens = [
    "AD_TD_SECTOR_SIZE 512UL",
    "AD_TD_SECTORS_PER_TRACK 11UL",
    "AD_TD_HEADS 2UL",
    "AD_TD_CYLINDERS 80UL",
    "ad_td_open",
    "ad_td_get_status",
    "ad_td_read_sector",
]
for token in required_header_tokens:
    if token not in header:
        errors.append(f"missing M1 API/geometry token: {token}")

for token in ["OpenDevice", "CloseDevice", "TD_CHANGESTATE", "TD_PROTSTATUS", "CMD_READ", "DoIO"]:
    if token not in source:
        errors.append(f"missing trackdisk implementation token: {token}")

# M1 itself remains read-only. Later milestones may extend the shared
# trackdisk backend with write support, so do not reject write tokens merely
# because they exist elsewhere in that backend. Keep the M1 CLI surface and
# its read primitive present and unchanged in intent.
if "ad_td_read_sector" not in source:
    errors.append("M1 read primitive is missing")

if "read-sector" not in main or "probe" not in main:
    errors.append("M1 diagnostic commands are missing")

if "src/io/trackdisk/trackdisk.c" not in makefile:
    errors.append("trackdisk source is not part of the native build")

if errors:
    for error in errors:
        print(f"FAIL: {error}")
    sys.exit(1)

print("M1 static checks: PASS")
