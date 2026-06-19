"""Rewrite OBJS_TEXT.md: one compact JSON line per image section."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

UFO_ROOT = Path(r"C:\Users\luisarandas\Desktop\UFO_FILES")
sys.path.insert(0, str(UFO_ROOT))

from detect_release_objects import slim_payload  # noqa: E402

SECTION_HEADER_RE = re.compile(r"^## `(.+?)`\s*$", re.MULTILINE)


def extract_json_block(section: str) -> str:
    marker = "```json"
    start = section.find(marker)
    if start == -1:
        return ""
    content_start = section.find("\n", start)
    if content_start == -1:
        return ""
    content_start += 1
    end = section.find("```", content_start)
    if end == -1:
        return section[content_start:]
    return section[content_start:end]


def compact_section(rel_path: str, json_text: str) -> str:
    payload = json.loads(json_text.strip())
    body = json.dumps(slim_payload(payload), ensure_ascii=False, separators=(",", ":"))
    return f"## `{rel_path}`\n```json\n{body}\n```\n\n"


def compact_markdown(text: str) -> tuple[str, int]:
    first_section = text.find("## `")
    if first_section == -1:
        return text, 0

    header = (
        "# Object, Text Region, and Color Detection\n\n"
        f"Source folder: `{UFO_ROOT / 'Release_1_PNG'}`\n\n"
        "One compact JSON line per image (`bbox` only; polygons/colors omitted).\n\n"
    )
    body = text[first_section:]
    parts = SECTION_HEADER_RE.split(body)
    # parts[0] is empty, then rel, section_body, rel, section_body, ...
    by_name: dict[str, str] = {}
    i = 1
    while i + 1 < len(parts):
        rel_path = parts[i].strip()
        section_body = parts[i + 1]
        json_text = extract_json_block(section_body)
        if not json_text.strip():
            i += 2
            continue
        by_name[Path(rel_path.replace("\\", "/")).name] = compact_section(rel_path, json_text)
        i += 2

    out_sections = list(by_name.values())
    return header + "".join(out_sections), len(out_sections)


def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: compact_objs_text.py <OBJS_TEXT.md> [output.md]", file=sys.stderr)
        return 1

    input_path = Path(sys.argv[1]).resolve()
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else input_path

    if not input_path.is_file():
        print(f"Not found: {input_path}", file=sys.stderr)
        return 1

    print(f"Reading {input_path}...", flush=True)
    text = input_path.read_text(encoding="utf-8", errors="replace")
    compacted, count = compact_markdown(text)
    if count == 0:
        print("No sections found.", file=sys.stderr)
        return 1

    tmp = output_path.with_suffix(output_path.suffix + ".tmp")
    tmp.write_text(compacted, encoding="utf-8")
    tmp.replace(output_path)
    line_count = len(compacted.splitlines())
    print(f"Compacted {count} section(s) -> {output_path} ({line_count} lines)", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
