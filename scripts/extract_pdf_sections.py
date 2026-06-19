"""Extract sections from PDF_TEXT.md to JSON for summarization."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

SECTION_RE = re.compile(r"^## `(.+?)`\s*$", re.MULTILINE)


def extract_json_block(section: str) -> str:
    marker = "```text"
    start = section.find(marker)
    if start == -1:
        return ""
    content_start = section.find("\n", start) + 1
    end = section.find("```", content_start)
    if end == -1:
        return section[content_start:].strip()
    return section[content_start:end].strip()


def main() -> int:
    src = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("bin/data/PDF_TEXT.md")
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("bin/data/_pdf_sections.json")
    text = src.read_text(encoding="utf-8")
    first = text.find("## `")
    body = text[first:] if first >= 0 else text
    parts = SECTION_RE.split(body)
    entries = []
    for i in range(1, len(parts), 2):
        entries.append({"rel": parts[i], "ocr": extract_json_block(parts[i + 1])})
    out.write_text(json.dumps(entries, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Wrote {len(entries)} entries to {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
