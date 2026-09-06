#!/usr/bin/env bash
# =========================================================================================================================
# TAILCAT ZER0 — Build Script for C99 Musl Static View-Only Shell
# Builds ultra-lean static binaries (~130 KB) using Zig CC cross-compilation
# =========================================================================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="${SCRIPT_DIR}/src/tailcat-view-shell.c"
BIN_DIR="${SCRIPT_DIR}/bin"

mkdir -p "$BIN_DIR"

build_target() {
    local target="$1"
    local outfile="$2"

    printf "  [*] Compiling \033[1;36m%s\033[0m for target \033[1;32m%s\033[0m (musl static)...\n" "$(basename "$outfile")" "$target"
    zig cc -target "${target}" -Wall -Wextra -Werror -pedantic -std=c99 -O3 -s "${SRC}" -o "${outfile}"
    printf "      \033[1;32m✔\033[0m Binary ready: %s (%s)\n" "$outfile" "$(du -h "${outfile}" | awk '{print $1}')"
}

build_native() {
    local outfile="${1:-${BIN_DIR}/tailcat-view-shell-native}"
    printf "  [*] Compiling \033[1;36m%s\033[0m (native host)...\n" "$(basename "$outfile")"
    if command -v zig >/dev/null 2>&1; then
        zig cc -Wall -Wextra -Werror -pedantic -std=c99 -O3 "${SRC}" -o "${outfile}"
    else
        ${CC:-cc} -Wall -Wextra -Werror -pedantic -std=c99 -O3 "${SRC}" -o "${outfile}"
    fi
    printf "      \033[1;32m✔\033[0m Native binary ready: %s (%s)\n" "$outfile" "$(du -h "${outfile}" | awk '{print $1}')"
}

case "${1:-all}" in
    native)
        build_native "${2:-${BIN_DIR}/tailcat-view-shell-native}"
        ;;
    armv7)
        build_target "arm-linux-musleabihf" "${BIN_DIR}/tailcat-view-shell-armv7"
        ;;
    arm64)
        build_target "aarch64-linux-musl" "${BIN_DIR}/tailcat-view-shell-arm64"
        ;;
    amd64)
        build_target "x86_64-linux-musl" "${BIN_DIR}/tailcat-view-shell-amd64"
        ;;
    all)
        printf "\n\033[1;36m========================================================================\033[0m\n"
        printf "  \033[1mBuilding TAILCAT ZER0 C99 View-Only Shell (Zig + Musl)\033[0m\n"
        printf "\033[1;36m========================================================================\033[0m\n\n"
        build_target "arm-linux-musleabihf" "${BIN_DIR}/tailcat-view-shell-armv7"
        build_target "aarch64-linux-musl"   "${BIN_DIR}/tailcat-view-shell-arm64"
        build_target "x86_64-linux-musl"    "${BIN_DIR}/tailcat-view-shell-amd64"
        build_native "${BIN_DIR}/tailcat-view-shell-native"
        printf "\n\033[1;32m🎉 All binaries compiled successfully!\033[0m\n\n"
        ;;
    *)
        echo "Usage: $0 [native|armv7|arm64|amd64|all]"
        exit 1
        ;;
esac
