#!/usr/bin/env python3
"""Build the unchanged original Windows game using its bundled x64 Raylib DLL."""
from pathlib import Path
import re
import shutil
import subprocess
import sys

from edk2 import ROOT, with_vs


def main():
    out = ROOT / "Build/windows-compat"
    out.mkdir(parents=True, exist_ok=True)
    with_vs([f'dumpbin /exports "{ROOT / "lib/raylib.dll"}" > "{out / "dll-exports.txt"}"'],
            arch="64", name="windows-exports")
    names = re.findall(r"^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\w+)",
                       (out / "dll-exports.txt").read_text(), re.M)
    if "InitWindow" not in names or "CloseWindow" not in names:
        raise RuntimeError("Raylib DLL exports were not found")
    (out / "raylib.def").write_text("LIBRARY raylib.dll\nEXPORTS\n" + "\n".join(names) + "\n")
    with_vs([f'lib /nologo /machine:x64 /def:"{out / "raylib.def"}" /out:"{out / "raylib.lib"}"',
             subprocess.list2cmdline(["cl", "/nologo", "/std:c11", "/MD", f"/I{ROOT / 'include'}",
                                     str(ROOT / "CampfireHunt.c"), f"/Fe{out / 'CampfireHunt.exe'}",
                                     f"/Fo{out / 'CampfireHunt.obj'}", "/link", str(out / "raylib.lib")])],
            arch="64", name="windows-build", cwd=out)
    if not (out / "CampfireHunt.exe").is_file():
        raise RuntimeError("Original Windows executable missing")
    shutil.copy2(ROOT / "lib/raylib.dll", out / "raylib.dll")
    print(f"Windows game compiled and linked: {out / 'CampfireHunt.exe'}")
    print("Run from the repository root so images/, audio/ and score files are found.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
