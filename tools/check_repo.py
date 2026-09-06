#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = [
    "README.md", "LICENSE", "Makefile", "src/main.c",
    "src/core/ad_version.c", "src/core/ad_version.h",
    "docs/ARCHITECTURE.md", "docs/ROADMAP.md",
]
errors = []
for name in REQUIRED:
    if not (ROOT / name).is_file():
        errors.append(f"missing required file: {name}")
makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
if "-m68000" not in makefile:
    errors.append("Makefile does not enforce 68000 baseline")
if not (ROOT / "LICENSE").read_text(encoding="utf-8").startswith("MIT License"):
    errors.append("LICENSE is not MIT")
for path in ROOT.rglob("*"):
    if not path.is_file() or ".git" in path.parts:
        continue
    if path.suffix in {".c", ".h", ".md", ".py"} or path.name in {"Makefile", "LICENSE", ".gitignore"}:
        data = path.read_bytes()
        if b"\r\n" in data:
            errors.append(f"CRLF: {path.relative_to(ROOT)}")
        if data and not data.endswith(b"\n"):
            errors.append(f"missing final newline: {path.relative_to(ROOT)}")
        for n, line in enumerate(data.decode("utf-8").splitlines(), 1):
            if line.rstrip() != line:
                errors.append(f"trailing whitespace: {path.relative_to(ROOT)}:{n}")
if errors:
    print("\n".join(f"FAIL: {e}" for e in errors))
    sys.exit(1)
print("M0 repository checks: PASS")
