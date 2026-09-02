#!/bin/sh
# =========================================================================================================================
# TailCat-Merlin Installer for Asuswrt-Merlin
# https://github.com/underd0se/tailcat-merlin
# =========================================================================================================================

set -eu

VERSION="v1.2.0"
TAILCAT_VER="v0.4.0"
REPO_RAW_URL="https://raw.githubusercontent.com/underd0se/tailcat-merlin/main"

# ANSI Colors
C_RESET="\033[0m"
C_BOLD="\033[1m"
C_GREEN="\033[1;32m"
C_CYAN="\033[1;36m"
C_RED="\033[1;31m"

INSTALL_SCRIPT="/jffs/scripts/tailcat"
ADDON_DIR="/jffs/addons/tailcat"
BIN_DIR="${ADDON_DIR}/bin"
TAILCAT_BIN="${BIN_DIR}/tailcat"
CFG_FILE="${ADDON_DIR}/tailcat.cfg"
AMTM_MOD="/jffs/addons/amtm/tailcat.mod"

printf "\n%b%b==============================================================%b\n" "$C_CYAN" "$C_BOLD" "$C_RESET"
printf "  %bTailCat-Merlin Installer %s%b\n" "$C_BOLD" "${VERSION}" "$C_RESET"
printf "  Ephemeral WireGuard Tunnel & File Drop Manager\n"
printf "%b==============================================================%b\n\n" "$C_CYAN" "$C_RESET"

# 1. Verification
if [ ! -d "/jffs/scripts" ]; then
    printf "%b[!] Error: /jffs/scripts directory not found.%b\n" "$C_RED" "$C_RESET"
    printf "This installer requires an Asuswrt-Merlin firmware environment with JFFS enabled.\n"
    exit 1
fi

# 2. Architecture Detection
arch=$(uname -m)
pkg_arch=""
case "$arch" in
    armv7*|armv6*|arm) pkg_arch="armv7" ;;
    aarch64*|arm64*)  pkg_arch="arm64" ;;
    *)
        printf "%b[!] Unsupported router CPU architecture: %s%b\n" "$C_RED" "${arch}" "$C_RESET"
        exit 1
        ;;
esac
printf "%b[+] Detected architecture: %b%s%b (%s)\n" "$C_GREEN" "$C_BOLD" "${pkg_arch}" "$C_RESET" "${arch}"

# 3. Directory Setup
mkdir -p "$ADDON_DIR" "$BIN_DIR"

# 4. Download TailCat Static Binary
download_url="https://github.com/tailscale/tailcat/releases/download/${TAILCAT_VER}/tailcat_${TAILCAT_VER#v}_linux_${pkg_arch}.tar.gz"
printf "%b[*] Downloading official TailCat binary (%s)...%b\n" "$C_CYAN" "${TAILCAT_VER}" "$C_RESET"
if curl -fsSL "$download_url" | tar -xz -C "$BIN_DIR" tailcat; then
    chmod 755 "$TAILCAT_BIN"
    printf "%b[+] TailCat binary installed at %s%b\n" "$C_GREEN" "$TAILCAT_BIN" "$C_RESET"
else
    printf "%b[!] Failed to download binary from GitHub.%b\n" "$C_RED" "$C_RESET"
    exit 1
fi

# 5. Install Main CLI Script
printf "%b[*] Installing CLI script to %s...%b\n" "$C_CYAN" "$INSTALL_SCRIPT" "$C_RESET"
if [ -f "./tailcat" ]; then
    cp -f "./tailcat" "$INSTALL_SCRIPT"
else
    curl -fsSL "${REPO_RAW_URL}/tailcat" -o "$INSTALL_SCRIPT"
fi
chmod 755 "$INSTALL_SCRIPT"

# 6. Default Configuration
if [ ! -f "$CFG_FILE" ]; then
    cat <<EOF > "$CFG_FILE"
TIMEOUT_MINUTES=30
DERP_URL="https://tailcat.dev/derpmap.json"
EOF
    printf "%b[+] Created default configuration: %s%b\n" "$C_GREEN" "$CFG_FILE" "$C_RESET"
fi

# 7. AMTM Menu Integration
if [ -d "/jffs/addons/amtm" ]; then
    cat <<'EOF' > "$AMTM_MOD"
# tailcat.mod for amtm
name="TailCat"
desc="Ephemeral WireGuard Tunnels"
exec="/jffs/scripts/tailcat"
EOF
    printf "%b[+] Registered TailCat into amtm menu.%b\n" "$C_GREEN" "$C_RESET"
fi

# 8. Reboot Teardown Hook in init-start
if [ -f "/jffs/scripts/init-start" ]; then
    if ! grep -q "tailcat_session" /jffs/scripts/init-start 2>/dev/null; then
        echo 'rm -f /tmp/tailcat_session.env /tmp/tailcat_addr.txt 2>/dev/null # TailCat cleanup' >> /jffs/scripts/init-start
    fi
fi

printf "\n%b%b==============================================================%b\n" "$C_GREEN" "$C_BOLD" "$C_RESET"
printf "  %b🎉 TailCat-Merlin Successfully Installed!%b\n" "$C_BOLD" "$C_RESET"
printf "%b==============================================================%b\n" "$C_GREEN" "$C_RESET"
printf "  To launch the TailCat Manager, simply run:\n"
printf "    %b%btailcat%b  or  %b%b/jffs/scripts/tailcat%b\n" "$C_CYAN" "$C_BOLD" "$C_RESET" "$C_CYAN" "$C_BOLD" "$C_RESET"
printf "%b==============================================================%b\n\n" "$C_GREEN" "$C_RESET"
