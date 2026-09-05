# 🚀 Installation & Getting Started

This guide details system requirements, prerequisites, one-line installation, post-install verification, updates, and complete uninstallation for **TAILCAT ZER0** on Asuswrt-Merlin routers.

---

## 📋 System Requirements & Compatibility

### 1. Supported Firmware
* **Asuswrt-Merlin** 386.x, 388.x, 3004.x, and 3006.x (including Gnuton Merlin builds for DSL/AX devices).
* Stock ASUS firmware is **not** supported because it lacks JFFS custom script execution and writable partition hooks.

### 2. Supported Architectures
The installer automatically inspects your router hardware architecture via `uname -m` and deploys the appropriate official Tailscale TailCat engine binary:
* **ARM64 (aarch64):** RT-AX86U, RT-AX88U (Pro), GT-AX6000, GT-AXE16000, GT-AX11000 Pro, RT-BE96U, RT-BE98U, etc.
* **ARMv7 (armv7l):** RT-AC68U, RT-AC86U, RT-AX56U, RT-AX58U, etc.
* **x86_64 (amd64):** Virtualized Merlin test environments and custom x86 builds.

### 3. Prerequisites
1. **Enable JFFS Custom Scripts:**
   - In the Asuswrt WebGUI, navigate to: **Administration ➔ System ➔ Persistent JFFS2 partition**.
   - Set **"Format JFFS partition at next boot"** to **No**.
   - Set **"Enable JFFS custom scripts and configs"** to **Yes**.
   - Click **Apply**.
2. **SSH Access Enabled:**
   - Navigate to: **Administration ➔ System ➔ SSH Daemon**.
   - Set **"Enable SSH"** to **LAN only** (or LAN + WAN if using keys).
   - Port default is `22` (or custom port such as `5522`).
3. **Internet Connectivity:**
   - The router must be able to resolve and reach `github.com`, `raw.githubusercontent.com`, and Tailscale's DERP relay endpoints over HTTPS/UDP.
4. **Optional: Entware:**
   - If Entware is installed (`amtm` ➔ `ep`), TAILCAT ZER0 automatically links into `/opt/bin/` and takes advantage of `qrencode` for crisp ASCII QR codes, as well as enhanced diagnostic tools like `rg` (ripgrep) and `tree`.

---

## ⚡ One-Line Quick Install

Log into your router via SSH and run the following command:

```sh
curl -fsSL https://raw.githubusercontent.com/underd0se/tailcat-zero/main/install.sh | sh
```

### What the Installer Does Automatically:
1. **Verifies JFFS Partition:** Ensures `/jffs` is mounted, writable, and has sufficient space (>15MB).
2. **Detects Architecture:** Identifies your CPU (`arm64`, `armv7`, or `amd64`).
3. **Fetches Engine Binary:** Downloads the official compiled `tailcat` engine directly from [Tailscale's GitHub Releases](https://github.com/tailscale/tailcat/releases/latest).
4. **Installs Scripts:**
   - Main manager: `/jffs/addons/tailcatzero/tailcatzero`
   - View-only sandbox: `/jffs/addons/tailcatzero/tailcat-view-shell`
   - Default configuration: `/jffs/addons/tailcatzero/tailcatzero.cfg`
5. **Registers PATH Symlinks:**
   - Creates `/jffs/scripts/tailcatzero`
   - Creates backward-compatible alias `/jffs/scripts/tailcat`
   - Links into `/opt/bin/tailcatzero` (if Entware is present).

---

## ✅ Post-Installation Verification

Confirm that TAILCAT ZER0 is correctly installed and in your system `$PATH`:

```sh
tailcatzero --version
```
*Expected output:*
```text
TAILCAT ZER0 v1.7.1 (engine v0.6.0)
```

Check the initial service status:
```sh
tailcatzero status
```
*Expected output:*
```text
[⚪] No active TAILCAT ZER0 sessions running.
```

Launch the interactive terminal dashboard:
```sh
tailcatzero
```

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  1. 🆘 Remote Support Shell         Full root or view-only access      [⚪ Inactive]
  2. 📥 Receive Files                Direct P2P file transfer           [⚪ Inactive]
  3. 📁 Share Directory (SFTP)       Share a folder from your drive     [⚪ Inactive]
  4. 🌐 Expose Router WebGUI         Access to router's web interface   [⚪ Inactive]

  ------------------------------------------------------------------------

  5. ⏱️ Configure Default Timeout    Current: 30 min
  6. ⚙️ Manage TAILCAT ZER0          Update, reinstall, or remove

========================================================================

  👁️ View Sessions  |  🛑 Stop  |  ↩️ Exit: 
```

---

## 🔄 Updates & Upgrades

TAILCAT ZER0 includes an intelligent dual-component updater that maintains both the management CLI scripts and the compiled TailCat engine binary.

### Updating via TUI:
Launch `tailcatzero`, select **Option 6 (Manage TAILCAT ZER0)**, and choose **Option 1 (Check & Update)**.

### Updating via CLI:
```sh
tailcatzero update
```

### Smart Version Comparison Logic:
* **CLI Script Update:** Fetches the latest script from GitHub. If an update is applied, the script automatically reloads the in-memory shell process cleanly.
* **Engine Binary Check:** Inspects the installed engine version against the latest GitHub release. If versions match, redundant downloads are skipped:

```text
========================================================================
  🐱 TAILCAT ZER0 — Dual-Component Update Check
========================================================================

  [1/2] Checking TAILCAT ZER0 Script:
        Current: v1.7.1 | Remote: v1.7.1
        [✓] Script is up to date.

  [2/2] Checking TailCat Engine Binary:
        Current: v0.6.0 | Remote: v0.6.0
        [✓] Engine binary is already up to date (v0.6.0).
```

---

## 🗑️ Uninstallation

If you ever wish to completely remove TAILCAT ZER0 from your router:

### Method 1: Via TUI
Launch `tailcatzero` ➔ select **Option 6 (Manage TAILCAT ZER0)** ➔ select **Option 3 (Complete Uninstall)**:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🗑️  Uninstall TAILCAT ZER0

  This will terminate all active sessions and clean up all TailCat files.

========================================================================

  🗑️ Yes, Uninstall  |  ↩️ Back / Cancel: 
```

Confirm with `y` to terminate all tunnels and purge all addon files.

### Method 2: Via Command Line
Run this cleanup sequence to stop all tunnels and remove files:
```sh
# Stop all running sessions
tailcatzero stop all 2>/dev/null

# Remove addon directories and scripts
rm -rf /jffs/addons/tailcatzero /jffs/addons/tailcat
rm -f /jffs/scripts/tailcatzero /jffs/scripts/tailcat
rm -f /opt/bin/tailcatzero /opt/bin/tailcat

# Clean volatile session locks and pipes
rm -rf /tmp/tailcat-* /tmp/tailcatzero-*
```

Your router's firewall rules and network configuration remain completely untouched and clean.
