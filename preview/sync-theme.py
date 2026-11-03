#!/usr/bin/env python3
"""Extract preview design tokens from D3X8's native Harp/Nuklear C sources."""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NK = ROOT / "userland/include/nk_harp.h"
DRAW = ROOT / "userland/files/harp/harp_draw.c"

def rgb_macro(source, name):
    pattern = r"#define\s+" + re.escape(name) + r"\s+nk_rgb\((\d+)\s*,\s*(\d+)\s*,\s*(\d+)\)"
    match = re.search(pattern, source)
    if not match:
        return None
    r, g, b = map(int, match.groups())
    return f"#{r:02x}{g:02x}{b:02x}"

nk = NK.read_text(encoding="utf-8")
draw = DRAW.read_text(encoding="utf-8")
accent = rgb_macro(nk, "ZEN_ACCENT") or "#0a84ff"
text = rgb_macro(nk, "ZEN_TEXT") or "#e6e6eb"
surface = rgb_macro(nk, "ZEN_SURFACE") or "#1a1a1c"
bg_match = re.search(r"#define\s+C_BG\s+0xFF([0-9A-Fa-f]{6})", draw)
bg = ("#" + bg_match.group(1).lower()) if bg_match else "#0e0e0e"

payload = {
    "source": {
        "theme": "userland/include/nk_harp.h",
        "desktop": "userland/files/harp/harp_draw.c"
    },
    "css": {
        "--accent": accent,
        "--native-text": text,
        "--native-surface": surface,
        "--native-desktop": bg
    }
}
out = Path(__file__).with_name("theme.json")
out.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
print("Generated preview/theme.json from native Harp C source.")
