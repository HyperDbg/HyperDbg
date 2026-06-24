#!/usr/bin/env python3
"""
Walks the parent directory of this script (and all of its subdirectories)
looking for .vcxproj files.
"""

import os

TARGET_SDK_WDK_VERSION = "10.0.28000.0" # Update this version in case of updating NuGet packages

# Only these .vcxproj files will be modified (matched by filename).
TARGET_PROJECTS = [
    "hyperhv.vcxproj",
    "hyperkd.vcxproj",
    "hypertrace.vcxproj",
    "hyperlog.vcxproj",
    "hyperevade.vcxproj",
    "hyperperf.vcxproj",
    "kdserial.vcxproj",
    "hyperdbg_driver.vcxproj",
]

REPLACEMENTS = {
    "<WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>": "<WindowsTargetPlatformVersion>" + TARGET_SDK_WDK_VERSION + "</WindowsTargetPlatformVersion>",
    "<WindowsTargetPlatformVersion>$(LatestTargetPlatformVersion)</WindowsTargetPlatformVersion>": "<WindowsTargetPlatformVersion>" + TARGET_SDK_WDK_VERSION + "</WindowsTargetPlatformVersion>",
}

# Directory *names* to skip wherever they appear in the tree.
EXCLUDED_DIR_NAMES = {".git"}

def replace_in_file(filepath: str) -> bool:
    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
 
    original = content
    for old, new in REPLACEMENTS.items():
        content = content.replace(old, new)
 
    if content != original:
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(content)
        return True
    return False
 
 
def main() -> None:
    script_path = os.path.abspath(__file__)
    script_dir = os.path.dirname(script_path)
    target_root = os.path.dirname(script_dir)  # parent of this folder
 
    wanted = set(TARGET_PROJECTS)
    found = set()
 
    print(f"Scanning under: {target_root}")
    print(f"Looking for: {', '.join(sorted(wanted))}")
 
    changed_count = 0
    for root, dirs, files in os.walk(target_root):
        # Prune in place so os.walk never descends into these directories.
        dirs[:] = [d for d in dirs if d not in EXCLUDED_DIR_NAMES]
 
        for file in files:
            if file in wanted:
                filepath = os.path.join(root, file)
                found.add(file)
 
                if replace_in_file(filepath):
                    print(f"Updated: {filepath}")
                    changed_count += 1
                else:
                    print(f"No matching text found, left unchanged: {filepath}")
 
    missing = wanted - found
    for name in sorted(missing):
        print(f"Warning: not found anywhere under {target_root}: {name}")
 
    print(f"Done. {changed_count} file(s) updated.")
 
 
if __name__ == "__main__":
    main()
