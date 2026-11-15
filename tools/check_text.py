"""Fail on unexpected BOM, invalid UTF-8, or selected invisible Unicode characters.

This is a source hygiene check, not proof of human authorship.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SUFFIXES = {".cpp", ".hpp", ".txt", ".md", ".py", ".yml", ".yaml", ".cmake", ".rfg"}
BAD = {"\u200b", "\u200c", "\u200d", "\u2060", "\ufeff", "\u202e"}
checked = 0
for path in ROOT.rglob("*"):
    if not path.is_file() or any(part.startswith("build") for part in path.relative_to(ROOT).parts):
        continue
    if path.suffix not in SUFFIXES and path.name not in {"CMakeLists.txt", ".gitignore", ".editorconfig"}:
        continue
    raw = path.read_bytes()
    if raw.startswith((b"\xef\xbb\xbf", b"\xff\xfe", b"\xfe\xff")):
        raise SystemExit(f"Unexpected BOM: {path}")
    text = raw.decode("utf-8", errors="strict")
    if any(char in BAD for char in text):
        raise SystemExit(f"Unexpected invisible character: {path}")
    checked += 1
print(f"PASS: {checked} UTF-8 text files checked (source hygiene only)")
