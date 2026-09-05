#!/bin/sh
# =========================================================================================================================
# TAILCAT ZER0 Installer for Asuswrt-Merlin
# https://github.com/underd0se/tailcat-zero
# =========================================================================================================================

set -eu

VERSION="v1.8.0"
REPO_RAW_URL="https://raw.githubusercontent.com/underd0se/tailcat-zero/main"

# ANSI Colors
C_RESET="\033[0m"
C_BOLD="\033[1m"
C_GREEN="\033[1;32m"
C_CYAN="\033[1;36m"
C_RED="\033[1;31m"

ADDON_DIR="/jffs/addons/tailcatzero"
INSTALL_SCRIPT="${ADDON_DIR}/tailcatzero"
BIN_DIR="${ADDON_DIR}/bin"
TAILCAT_BIN="${BIN_DIR}/tailcat"
VIEW_SHELL_BIN="${BIN_DIR}/tailcat-view-shell"
CFG_FILE="${ADDON_DIR}/tailcatzero.cfg"

printf "\n%b%b==============================================================%b\n" "$C_CYAN" "$C_BOLD" "$C_RESET"
printf "  %bTAILCAT ZER0 Installer %s%b\n" "$C_BOLD" "${VERSION}" "$C_RESET"
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
    x86_64*|amd64*)   pkg_arch="amd64" ;;
    *)
        printf "%b[!] Unsupported router CPU architecture: %s%b\n" "$C_RED" "${arch}" "$C_RESET"
        exit 1
        ;;
esac
printf "%b[+] Detected architecture: %b%s%b (%s)\n" "$C_GREEN" "$C_BOLD" "${pkg_arch}" "$C_RESET" "${arch}"

# 3. Directory Setup
mkdir -p "$ADDON_DIR" "$BIN_DIR"

# 4. Download TailCat Static Binary (100% Dynamic Discovery)
cur_bin_ver=""
if [ -x "$TAILCAT_BIN" ]; then
    cur_bin_ver=$("$TAILCAT_BIN" --version 2>/dev/null || true)
fi

latest_tag=$(curl -sI -m 6 https://github.com/tailscale/tailcat/releases/latest 2>/dev/null | grep -i '^location:' | sed -e 's/.*tag\///' -e 's/[[:space:]\r\n]//g' || true)
if [ -z "$latest_tag" ]; then
    latest_tag=$(curl -fsSL -m 6 -H "Accept: application/vnd.github.v3+json" https://api.github.com/repos/tailscale/tailcat/releases/latest 2>/dev/null | grep '"tag_name":' | head -n 1 | cut -d'"' -f4 || echo "")
fi

if [ -z "$latest_tag" ]; then
    if [ -n "$cur_bin_ver" ]; then
        printf "%b[!] Could not query latest release tag from GitHub. Retaining existing binary (%s).%b\n" "$C_YELLOW" "$cur_bin_ver" "$C_RESET"
    else
        printf "%b[!] Error: Could not determine latest TailCat release from GitHub. Check internet connection.%b\n" "$C_RED" "$C_RESET"
        exit 1
    fi
elif [ -n "$cur_bin_ver" ] && [ "$cur_bin_ver" = "$latest_tag" ]; then
    printf "%b[✓] TailCat binary is already up to date (%s). Skipping download.%b\n" "$C_GREEN" "$cur_bin_ver" "$C_RESET"
else
    download_url="https://github.com/tailscale/tailcat/releases/download/${latest_tag}/tailcat_${latest_tag#v}_linux_${pkg_arch}.tar.gz"
    printf "%b[*] Downloading official TailCat engine binary (%s)...%b\n" "$C_CYAN" "${latest_tag}" "$C_RESET"
    if curl -fsSL "$download_url" | tar -xz -C "$BIN_DIR" tailcat 2>/dev/null; then
        chmod 755 "$TAILCAT_BIN"
        printf "%b[+] TailCat binary installed at %s (%s)%b\n" "$C_GREEN" "$TAILCAT_BIN" "$latest_tag" "$C_RESET"
    elif [ -n "$cur_bin_ver" ]; then
        printf "%b[!] Download failed. Retaining current working binary (%s).%b\n" "$C_YELLOW" "$cur_bin_ver" "$C_RESET"
    else
        printf "%b[!] Failed to download binary from GitHub.%b\n" "$C_RED" "$C_RESET"
        exit 1
    fi
fi

# 5. Dependency Verification (qrencode via firmware or Entware)
qr_cmd=""
if command -v qrencode >/dev/null 2>&1; then
    qr_cmd="qrencode"
elif [ -x "/usr/sbin/qrencode" ]; then
    qr_cmd="/usr/sbin/qrencode"
elif [ -x "/opt/bin/qrencode" ]; then
    qr_cmd="/opt/bin/qrencode"
fi

if [ -z "$qr_cmd" ] && [ -x "/opt/bin/opkg" ]; then
    printf "%b[*] Installing qrencode via Entware (opkg)...%b\n" "$C_CYAN" "$C_RESET"
    /opt/bin/opkg update >/dev/null 2>&1 || true
    /opt/bin/opkg install qrencode >/dev/null 2>&1 || true
fi

# 6. Install Main CLI Script & View-Only Shell
printf "%b[*] Installing CLI script to %s...%b\n" "$C_CYAN" "$INSTALL_SCRIPT" "$C_RESET"
if [ -f "./tailcatzero" ]; then
    cp -f "./tailcatzero" "$INSTALL_SCRIPT"
elif [ -f "./tailcat" ]; then
    cp -f "./tailcat" "$INSTALL_SCRIPT"
else
    curl -fsSL "${REPO_RAW_URL}/tailcatzero" -o "$INSTALL_SCRIPT"
fi
chmod 755 "$INSTALL_SCRIPT"

if [ -f "./tailcat-view-shell" ]; then
    cp -f "./tailcat-view-shell" "$VIEW_SHELL_BIN"
else
    curl -fsSL "${REPO_RAW_URL}/tailcat-view-shell" -o "$VIEW_SHELL_BIN"
fi
chmod 755 "$VIEW_SHELL_BIN"

# Clean up legacy directories and files from /jffs/scripts and /jffs/addons/tailcat
rm -rf "/jffs/scripts/tailcat" "/jffs/scripts/tailcat-merlin" 2>/dev/null || true
if [ -d "/jffs/addons/tailcat" ]; then
    if [ -f "/jffs/addons/tailcat/tailcat.cfg" ] && [ ! -f "$CFG_FILE" ]; then
        cp -f "/jffs/addons/tailcat/tailcat.cfg" "$CFG_FILE" 2>/dev/null || true
    fi
    rm -rf "/jffs/addons/tailcat" 2>/dev/null || true
fi

# Create symlink for tailcatzero in /jffs/scripts and /opt/bin
mkdir -p /jffs/scripts
ln -sf "$INSTALL_SCRIPT" "/jffs/scripts/tailcatzero"

if [ -d "/opt/bin" ]; then
    ln -sf "$INSTALL_SCRIPT" "/opt/bin/tailcatzero"
    rm -f "/opt/bin/tailcat" "/opt/bin/tailcat-merlin" 2>/dev/null || true
fi
printf "%b[+] Registered command: %b%s%b\n" "$C_GREEN" "$C_BOLD" "tailcatzero" "$C_RESET"

# 7. Default Configuration
if [ ! -f "$CFG_FILE" ]; then
    cat <<EOF > "$CFG_FILE"
TIMEOUT_MINUTES=30
DERP_URL="https://tailcat.dev/derpmap.json"
EOF
    printf "%b[+] Created default configuration: %s%b\n" "$C_GREEN" "$CFG_FILE" "$C_RESET"
fi

# 8. Reboot Teardown Hook in init-start
if [ ! -f "/jffs/scripts/init-start" ]; then
    printf "#!/bin/sh\n\n" > "/jffs/scripts/init-start"
fi
chmod 755 "/jffs/scripts/init-start"
if ! grep -q "tailcat_sessions" /jffs/scripts/init-start 2>/dev/null; then
    echo 'rm -rf /tmp/tailcat_sessions /tmp/tailcat_addr*.txt 2>/dev/null # TAILCAT ZER0 cleanup' >> /jffs/scripts/init-start
fi

printf "\n%b%b==============================================================%b\n" "$C_GREEN" "$C_BOLD" "$C_RESET"
printf "  %b🎉 TAILCAT ZER0 Successfully Installed!%b\n" "$C_BOLD" "$C_RESET"
printf "%b==============================================================%b\n" "$C_GREEN" "$C_RESET"
printf "  To launch the TAILCAT ZER0 Manager, simply run:\n"
printf "    %b%btailcatzero%b\n" "$C_CYAN" "$C_BOLD" "$C_RESET"
printf "%b==============================================================%b\n\n" "$C_GREEN" "$C_RESET"
