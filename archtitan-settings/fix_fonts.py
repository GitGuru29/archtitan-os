#!/usr/bin/env python3
import re
import sys
import pathlib

QML_DIR = pathlib.Path("/home/msfvenom/Desktop/archtitan-settings/qml")

# We want to replace something like:
# font { pixelSize: 14; family: "Inter" }
# with:
# font { pixelSize: 14 + (SettingsBackend.fontSize - 13); family: SettingsBackend.fontFamily }

# We might also have:
# font.family: "Inter"
# font.pixelSize: 14

def process_file(path):
    text = path.read_text(encoding="utf-8")
    original_text = text
    
    # Replace inline font group properties
    # e.g., font { pixelSize: 14; family: "Inter" }
    
    # Step 1: replace family: "Inter" with family: SettingsBackend.fontFamily
    text = re.sub(r'family:\s*"Inter"', r'family: SettingsBackend.fontFamily', text)
    
    # Step 2: replace pixelSize: X with pixelSize: X + (SettingsBackend.fontSize - 13)
    # We must be careful not to replace it if it already has SettingsBackend in it
    def repl_pixelSize(m):
        size = m.group(1)
        # If it's a ternary or expression, just leave it alone or append to it if simple
        if not size.isdigit():
            return m.group(0)
        return f"pixelSize: {size} + (SettingsBackend.fontSize - 13)"
        
    text = re.sub(r'pixelSize:\s*(\d+)', repl_pixelSize, text)
    
    if text != original_text:
        path.write_text(text, encoding="utf-8")
        print(f"Updated {path.name}")
        return True
    return False

def main():
    changed = 0
    for qml in QML_DIR.rglob("*.qml"):
        if process_file(qml):
            changed += 1
            
    print(f"Done. Updated {changed} QML files.")

if __name__ == "__main__":
    main()
