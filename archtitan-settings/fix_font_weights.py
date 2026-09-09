#!/usr/bin/env python3
"""
Fix Qt6 QML font group weight issue:
  font { pixelSize: N; weight: Font.X; family: "Y" }
  →
  font { pixelSize: N; family: "Y" }
  font.weight: Font.X

Also handles:
  font { pixelSize: N; weight: Font.X; family: "Y"; letterSpacing: Z }
  →
  font { pixelSize: N; family: "Y" }
  font.weight: Font.X
  font.letterSpacing: Z
"""
import re, sys, pathlib

QML_DIR = pathlib.Path("/home/msfvenom/Desktop/archtitan-settings/qml")

# Pattern: font { ... weight: Font.X ... }  (single line)
PATTERN = re.compile(
    r'^(\s*)font \{([^}]*)\}',
    re.MULTILINE
)

def fix_font_group(m):
    indent = m.group(1)
    inner = m.group(2)

    # Extract weight
    weight_m = re.search(r'\bweight:\s*(Font\.\w+)', inner)
    # Extract letterSpacing
    ls_m = re.search(r'\bletterSpacing:\s*([\d.]+)', inner)

    # Remove weight and letterSpacing from inner
    cleaned = re.sub(r';\s*weight:\s*Font\.\w+', '', inner)
    cleaned = re.sub(r'weight:\s*Font\.\w+\s*;?\s*', '', cleaned)
    cleaned = re.sub(r';\s*letterSpacing:\s*[\d.]+', '', cleaned)
    cleaned = re.sub(r'letterSpacing:\s*[\d.]+\s*;?\s*', '', cleaned)
    # Tidy up double semicolons/spaces
    cleaned = re.sub(r';\s*;', ';', cleaned)
    cleaned = cleaned.strip().rstrip(';').strip()

    result = f"{indent}font {{ {cleaned} }}"
    if weight_m:
        result += f"\n{indent}font.weight: {weight_m.group(1)}"
    if ls_m:
        result += f"\n{indent}font.letterSpacing: {ls_m.group(1)}"
    return result

changed = 0
for qml in QML_DIR.rglob("*.qml"):
    text = qml.read_text()
    new_text = PATTERN.sub(fix_font_group, text)
    if new_text != text:
        qml.write_text(new_text)
        changed += 1
        print(f"  Fixed: {qml.relative_to(QML_DIR)}")

print(f"\nDone — {changed} file(s) updated.")
