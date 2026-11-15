#!/usr/bin/env python3
"""Build the actual Campfire Hunt core with an external EDK II (Windows/VS2022)."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = "CampfireHuntPkg/CampfireHuntPkg.dsc"
MODULE = ROOT / "CampfireHuntPkg/Application/CampfireHunt"
TESTED_EDK2 = "37b18eca7958215fcf81b297e313adf60c0a70cb"


def git_head(edk):
    # One read-only invocation; no persistent safe.directory/config changes.
    return subprocess.check_output(["git", "-c", f"safe.directory={edk.as_posix()}",
                                    "-C", str(edk), "rev-parse", "HEAD"], text=True).strip()


def run(command, *, cwd=ROOT, env=None):
    print("+", subprocess.list2cmdline([str(x) for x in command]), flush=True)
    return subprocess.run([str(x) for x in command], cwd=cwd, env=env, check=True)


def vs_script(arch="32"):
    vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
    result = subprocess.check_output([
        str(vswhere), "-latest", "-products", "*", "-version", "[17.0,18.0)",
        "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "-property", "installationPath"], text=True).strip()
    script = Path(result) / f"VC/Auxiliary/Build/vcvars{arch}.bat"
    if not result or not script.is_file():
        raise RuntimeError(f"VS2022 C++ vcvars{arch}.bat was not found")
    return script


def edk_path(value):
    value = value or os.environ.get("EDK2_DIR")
    path = Path(value) if value else ROOT.parent / "edk2-vscode-starter/third_party/edk2"
    path = path.resolve()
    if not (path / "edksetup.bat").is_file():
        raise RuntimeError("External EDK II missing: pass --edk2 PATH or set EDK2_DIR. See README_UEFI.md")
    for p in (ROOT, path):
        if any(c in str(p) for c in ' &()^%!'):
            raise RuntimeError(f"EDK II workspace/package paths must not contain spaces or cmd metacharacters: {p}")
    return path


def environment(edk):
    env = os.environ.copy()
    env.update(WORKSPACE=str(ROOT), PACKAGES_PATH=os.pathsep.join((str(ROOT), str(edk))),
               EDK_TOOLS_PATH=str(edk / "BaseTools"), BASE_TOOLS_PATH=str(edk / "BaseTools"),
               CONF_PATH=str(ROOT / "Conf"), PYTHON_COMMAND=sys.executable,
               HOST_ARCH="IA32", VSLANG="1033")
    nasm = shutil.which("nasm")
    if not nasm:
        candidate = Path(os.environ.get("ProgramFiles", r"C:\Program Files")) / "NASM/nasm.exe"
        if candidate.is_file():
            nasm = str(candidate)
    if not nasm:
        raise RuntimeError("NASM not found (required by X64 BaseLib)")
    env["NASM_PREFIX"] = str(Path(nasm).parent) + os.sep
    return env


def configure(edk):
    conf = ROOT / "Conf"
    conf.mkdir(exist_ok=True)
    for stem in ("target", "tools_def", "build_rule"):
        target = conf / f"{stem}.txt"
        if not target.exists():
            shutil.copy2(edk / f"BaseTools/Conf/{stem}.template", target)


def with_vs(commands, *, env=None, cwd=ROOT, arch="32", name="edk2"):
    # Batch syntax quotes paths directly: do not use C-style backslash quotes.
    launcher = ROOT / "Build" / f"_{name}.cmd"
    launcher.parent.mkdir(exist_ok=True)
    lines = ["@echo off", f'call "{vs_script(arch)}" >nul', "if errorlevel 1 exit /b %errorlevel%"]
    for command in commands:
        lines += [command, "if errorlevel 1 exit /b %errorlevel%"]
    lines += ["exit /b 0"]
    launcher.write_text("\n".join(lines) + "\n", encoding="utf-8")
    run(["cmd.exe", "/d", "/c", str(launcher)], cwd=cwd, env=env)


def verify(path):
    data = path.read_bytes()
    if len(data) < 64 or data[:2] != b"MZ":
        raise RuntimeError(f"Not a DOS/PE image: {path}")
    offset = struct.unpack_from("<I", data, 0x3c)[0]
    if offset + 24 + 112 > len(data) or data[offset:offset + 4] != b"PE\0\0":
        raise RuntimeError(f"Invalid PE header: {path}")
    machine, sections = struct.unpack_from("<HH", data, offset + 4)
    optional_size = struct.unpack_from("<H", data, offset + 20)[0]
    optional = offset + 24
    magic = struct.unpack_from("<H", data, optional)[0]
    entry = struct.unpack_from("<I", data, optional + 16)[0]
    subsystem = struct.unpack_from("<H", data, optional + 68)[0]
    imports = struct.unpack_from("<II", data, optional + 120)
    if (machine, magic, subsystem) != (0x8664, 0x20b, 10) or not entry or not sections:
        raise RuntimeError(f"Expected AMD64 PE32+ EFI Application with entry point: {path}")
    if imports != (0, 0):
        raise RuntimeError(f"UEFI application unexpectedly imports DLLs: {path}")
    for i in range(sections):
        section = optional + optional_size + i * 40
        raw_size, raw_offset = struct.unpack_from("<II", data, section + 16)
        if raw_offset + raw_size > len(data):
            raise RuntimeError(f"Truncated PE section: {path}")
    result = dict(file=str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path),
                  size=len(data), machine=hex(machine), subsystem=subsystem,
                  pe32plus=True, imports=0, entry=hex(entry), sha256=hashlib.sha256(data).hexdigest())
    print(json.dumps(result, indent=2))
    return result


def artifact(target):
    # Exact path only. Never fall back to another target or HelloWorld.
    return ROOT / f"Build/CampfireHunt/{target}_VS2022/X64/CampfireHunt.efi"


def bootstrap(edk, env):
    configure(edk)
    # EDK II toolsetup's internal nmake can lose its exit status through popd.
    # nmake is the final checked command in our launcher instead.
    with_vs([f'call "{edk / "edksetup.bat"}" VS2022', "nmake /nologo c"],
            cwd=edk / "BaseTools", env=env)
    for tool in ("GenFw", "GenFfs", "GenFv", "VfrCompile"):
        path = edk / f"BaseTools/Bin/Win32/{tool}.exe"
        if not path.is_file() or path.stat().st_size == 0:
            raise RuntimeError(f"BaseTools executable missing: {path}")
    print("BaseTools bootstrap OK (IA32 host tools; X64 firmware target)")


def build(edk, env, target, jobs):
    configure(edk)
    path = artifact(target)
    # Delete only this generated final image: an old image cannot prove success.
    if path.exists():
        path.unlink()
    started = time.time()
    # EDK II's incremental cache may skip a module when only the copied final
    # image is missing. Its native module clean forces compilation and linking.
    clean = ["build", "-p", PACKAGE, "-m", "CampfireHuntPkg/Application/CampfireHunt/CampfireHunt.inf",
             "-a", "X64", "-b", target, "-t", "VS2022", "clean"]
    module_build = path.parent / "CampfireHuntPkg/Application/CampfireHunt/CampfireHunt"
    commands = [f'call "{edk / "edksetup.bat"}" VS2022']
    # A fresh workspace has no generated makefiles for EDK II's clean target.
    if (module_build / "Makefile").is_file():
        commands.append("call " + subprocess.list2cmdline(clean))
    commands.append("call " + subprocess.list2cmdline(["build", "-p", PACKAGE, "-a", "X64", "-b", target,
                    "-t", "VS2022", "-n", str(jobs), "-y", f"Build/{target}-report.txt"]))
    with_vs(commands, env=env)
    if not path.is_file() or path.stat().st_mtime < started - 2:
        raise RuntimeError(f"Build did not produce a fresh artifact: {path}")
    makefile = (module_build / "Makefile").read_text(errors="replace")
    if "GameCore.c" not in makefile or not (module_build / "OUTPUT/GameCore.obj").is_file():
        raise RuntimeError("GameCore.c was not compiled into the module")
    # EDK II packages both object files into this library before the final link.
    libraries = list((module_build / "OUTPUT").glob("*.lib"))
    if len(libraries) != 1:
        raise RuntimeError("Module link library was not generated")
    with_vs([f'lib /nologo /list "{libraries[0]}" > "{module_build / "objects.txt"}"'], env=env)
    if "gamecore.obj" not in (module_build / "objects.txt").read_text().lower():
        raise RuntimeError("GameCore.obj is missing from the module link library")
    map_text = (module_build / "DEBUG/CampfireHunt.map").read_text(errors="replace")
    for symbol in ("GenerateGoblinPosition", "GenerateMonMonPosition", "UpdateObjects", "InitWorld", "UpdateGame", "UpdateDeathAnimations"):
        if not any(symbol in line and "GameCore.obj" in line for line in map_text.splitlines()):
            raise RuntimeError(f"Linked gameplay symbol missing from map: {symbol}")
    result = verify(path)
    result.update(target=target, edk2_commit=git_head(edk),
        sources={p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(MODULE.glob("*")) if p.suffix in (".c", ".h", ".inf")})
    (path.parent / "verification.json").write_text(json.dumps(result, indent=2) + "\n")


def test():
    output = ROOT / "Build/tests"
    output.mkdir(parents=True, exist_ok=True)
    run([sys.executable, ROOT / "tests/extract_reference.py"])
    with_vs([subprocess.list2cmdline([
        "cl", "/nologo", "/std:c11", "/W4", "/WX", "/Od", "/Zi", "/RTC1", "/MDd",
        f"/I{MODULE}", str(MODULE / "GameCore.c"), str(ROOT / "tests/game_core_test.c"),
        f"/Fe{output / 'game_core_tests.exe'}", f"/Fo{output}\\", f"/Fd{output / 'tests.pdb'}"]),
        f'"{output / "game_core_tests.exe"}"'], cwd=output, arch="64", name="tests")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("doctor", "bootstrap", "build", "verify", "test"))
    parser.add_argument("--edk2", help="External EDK II checkout (or EDK2_DIR)")
    parser.add_argument("--target", choices=("DEBUG", "RELEASE"), default="DEBUG")
    parser.add_argument("-j", "--jobs", type=int, default=max(1, min(8, os.cpu_count() or 1)))
    args = parser.parse_args()
    if args.command == "verify":
        verify(artifact(args.target))
    elif args.command == "test":
        test()
    else:
        edk = edk_path(args.edk2)
        env = environment(edk)
        if args.command == "doctor":
            print(f"Python {sys.version.split()[0]}; VS2022: {vs_script()}; NASM: {env['NASM_PREFIX']}")
            print(f"EDK II: {edk}; HEAD=" + git_head(edk))
            print(f"Tested commit: {TESTED_EDK2}; host=IA32; target=X64; package={PACKAGE}")
        elif args.command == "bootstrap":
            bootstrap(edk, env)
        else:
            build(edk, env, args.target, args.jobs)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError, struct.error) as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(exc.returncode if isinstance(exc, subprocess.CalledProcessError) else 1)
