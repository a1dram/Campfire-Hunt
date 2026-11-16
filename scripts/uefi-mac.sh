#!/usr/bin/env bash
# Build and run Campfire Hunt UEFI on macOS (Apple Silicon or Intel).
#
#   scripts/uefi-mac.sh setup            # brew deps + build BaseTools (once)
#   scripts/uefi-mac.sh build [DEBUG|RELEASE]
#   scripts/uefi-mac.sh run   [DEBUG|RELEASE]   # boots the EFI in QEMU + OVMF
#
# Toolchain: CLANGPDB (Homebrew LLVM clang + lld-link) -> native PE32+ EFI.
# EDK II location: $EDK2_DIR, default third_party/edk2 (git-ignored).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EDK2_DIR="${EDK2_DIR:-$ROOT/third_party/edk2}"
BREW="$(brew --prefix)"
CMD="${1:-build}"
TARGET="${2:-DEBUG}"
EFI="$ROOT/Build/CampfireHunt/${TARGET}_CLANGPDB/X64/CampfireHunt.efi"

die() { echo "error: $*" >&2; exit 1; }
[ -f "$EDK2_DIR/edksetup.sh" ] || die "EDK II not found at $EDK2_DIR (set EDK2_DIR)"

setup() {
  brew install nasm llvm lld qemu
  make -C "$EDK2_DIR/BaseTools" -j"$(sysctl -n hw.ncpu)"
}

build() {
  [ -x "$EDK2_DIR/BaseTools/Source/C/bin/GenFw" ] || die "BaseTools not built: run '$0 setup'"
  # EDK II expects clang and lld-link in one CLANG_BIN dir; Homebrew splits them.
  local bin="$ROOT/Build/clang-bin"
  mkdir -p "$bin"
  for t in "$BREW"/opt/llvm/bin/*; do ln -sf "$t" "$bin/"; done
  for t in "$BREW"/opt/lld/bin/*;  do ln -sf "$t" "$bin/"; done
  export CLANG_BIN="$bin/" NASM_PREFIX="$BREW/bin/"
  export WORKSPACE="$ROOT" PACKAGES_PATH="$ROOT:$EDK2_DIR"
  export EDK_TOOLS_PATH="$EDK2_DIR/BaseTools"
  cd "$ROOT"
  # edksetup.sh parses "$@" and is not 'set -u' clean: source it with no args.
  set +u; set --; . "$EDK2_DIR/edksetup.sh" >/dev/null; set -u
  rm -f "$EFI"
  build_cmd=(build -p CampfireHuntPkg/CampfireHuntPkg.dsc -a X64 -t CLANGPDB -b "$TARGET" -n "$(sysctl -n hw.ncpu)")
  "${build_cmd[@]}"
  [ -f "$EFI" ] || die "build did not produce $EFI"
  echo "OK: $EFI ($(stat -f%z "$EFI") bytes)"
}

run() {
  [ -f "$EFI" ] || build
  local esp="$ROOT/.qemu/esp"
  rm -rf "$esp"; mkdir -p "$esp/EFI/BOOT"
  cp "$EFI" "$esp/CampfireHunt.efi"
  # UEFI Shell runs startup.nsh: launch the game, then power off QEMU.
  printf 'fs0:\r\nCampfireHunt.efi\r\nreset -s\r\n' > "$esp/startup.nsh"
  echo "Booting OVMF in QEMU (Ctrl+A then X to quit)..."
  qemu-system-x86_64 -machine q35 -m 256M -net none -nographic \
    -drive if=pflash,format=raw,readonly=on,file="$BREW/share/qemu/edk2-x86_64-code.fd" \
    -drive format=raw,file=fat:rw:"$esp"
}

case "$CMD" in
  setup) setup ;;
  build) build ;;
  run)   run ;;
  *) die "usage: $0 setup|build|run [DEBUG|RELEASE]" ;;
esac
