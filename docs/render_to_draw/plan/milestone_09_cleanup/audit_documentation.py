"""Nonexecuting maintained-documentation checks for the final draw production API."""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import os
import re


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def anchors(text):
    identifiers = set()
    counts = {}
    for heading in re.findall(r"^#{1,6}\s+(.+)", text, re.M):
        slug = "".join(c for c in heading.lower().strip() if c.isalnum() or c in " _-").replace(" ", "-")
        count = counts.get(slug, 0)
        counts[slug] = count + 1
        identifiers.add(slug + ("-" + str(count) if count else ""))
    identifiers.update(re.findall(r'<a\s+(?:id|name)=["\']([^"\']+)', text))
    return identifiers


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    pages = sorted(Path("doc").rglob("*.md"))
    errors = []
    links = 0
    includes = 0
    illustrative = []
    for page in pages:
        text = page.read_text()
        prose = re.sub(r"```.*?```", "", text, flags=re.S)
        for destination in re.findall(r"\]\(([^)\s]+)\)", prose):
            if ":" in destination:
                continue
            links += 1
            name, _, anchor = destination.partition("#")
            target = page.parent / name if name else page
            if not target.exists():
                errors.append(f"{page}: missing {destination}")
            elif anchor and target.is_file():
                body = re.sub(r"```.*?```", "", target.read_text(), flags=re.S)
                if anchor not in anchors(body):
                    errors.append(f"{page}: missing anchor {destination}")
        for header in re.findall(r'#include\s*[<"](bg2e/[^>"\n]+)[>"]', text):
            includes += 1
            if not (Path("lib/include") / header).is_file():
                if header == "bg2e/scene/MyComponent.hpp" and "api/reflection/" in str(page):
                    illustrative.append({"page": str(page), "header": header, "reason": "user-defined sample component"})
                else:
                    errors.append(f"{page}: missing include {header}")
        if re.search(r"bg2e::render\b|bg2e/render/|RendererBasicForward|scene::vk", text):
            # The scene index may identify the removed binding API as a breaking change.
            if "scene::vk" in text and str(page) == "doc/api/scene/index.md" and not re.search(r"bg2e::render\b|bg2e/render/|RendererBasicForward", text):
                continue
            errors.append(f"{page}: retired API spelling")
    report = {
        "kind": "nonexecuting_documentation_audit",
        "pages": len(pages), "local_links_and_anchors": links,
        "project_include_occurrences": includes,
        "illustrative_header_exceptions": illustrative,
        "errors": errors,
        "page_set_sha256": hashlib.sha256("".join(f"{p}\t{digest(p)}\n" for p in pages).encode()).hexdigest(),
        "limits": "Markdown links/heading slugs and project includes only; no C++ parse, external URL fetch, build, runtime, package or binary validation.",
    }
    (args.output_dir / "documentation-audit.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    if errors:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
