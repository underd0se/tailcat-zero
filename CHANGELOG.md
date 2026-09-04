# Changelog ─ TAILCAT ZER0

All notable changes to TAILCAT ZER0 are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.6.0] - 2026-09-04

### 🔒 Restricted View-Only Diagnostic Shell, Entware Inspection & 5-Slot Multi-Service Concurrency

* **🔒 Restricted View-Only Diagnostic Shell (`tailcat-view-shell`):** Introduced a zero-trust, read-only remote support shell option (`Option 1 -> 2` or `tailcatzero view` / `tailcatzero ssh view`). Clients connect using standard `tailcat ssh <token>` while the session is strictly restricted to safe diagnostic inspection.
* **🛡️ Hardened Multi-Layer Security Sandbox:** Prohibits file redirections (`>`, `>>`, `<`), subshells (`` ` `` / `$()`), command chaining (`;`, `&&`, `||`), state-modifying binaries (`rm`, `mv`, `cp`, `touch`, `chmod`, `dd`), router configuration changes (`nvram set/commit/unset`, `reboot`, `kill`), and package mutations (`opkg install/remove/upgrade`).
* **📦 Deep Entware & Asuswrt Diagnostics:** Permits comprehensive read-only tools across system health (`uptime`, `free`, `df`, `ps`, `top`, `dmesg`, `sysinfo`), networking & WiFi (`ip addr/route`, `netstat`, `route`, `ping`, `mtr`, `wl`, `leases`, `wifi`, `ports`), NVRAM queries (`nvram get`, `nvram show`), text processing (`cat`, `head`, `tail`, `grep`, `rg`, `tree`, `sort`, `uniq`, `diff`), and Entware queries (`opkg list/info/find/status/search/depends`).
* **⚡ Safe Pipeline Execution:** Supports Unix pipelines (`|`) between allowed tools (e.g. `ps | grep dnsmasq`, `nvram show | grep dhcp`, `opkg list-installed | grep python`).
* **🆘 Remote Support Shell Options Menu (Option 1):** Main dashboard Option 1 opens a clean submenu allowing admins to choose between `1. Full Root Shell (Read-Write)` and `2. View-Only Diagnostic Shell (Read-Only)`. Features dynamic visual badge indicators (`[🟢 Root + 🔒 View]`).
* **🖐️ 5-Slot Multi-Service Concurrency:** Upgraded active session tracking, multi-session card overview, and selective kill confirmations to support 5 concurrent service slots (`svc_1` to `svc_5`).

---

## [1.5.0] - 2026-09-04

### 🚀 CLI Timeout Management, Multi-Arch (`amd64`), DERP_URL Preservation & Self-Healing Resilience

* **⏱️ CLI Timeout Management (`tailcatzero timeout`):** Direct command-line inspection and configuration of the auto-kill timeout (`tailcatzero timeout [min|persistent]`) without entering interactive TUI menus. Supports `persistent` or `0` for persistent mode.
* **🛡️ DERP Map URL Preservation:** Implemented `save_timeout_config` helper to preserve existing `DERP_URL` definitions in `/jffs/addons/tailcat/tailcat.cfg` when updating session timeouts.
* **💻 Universal Architecture Expansion (`amd64` / `x86_64`):** Added 64-bit x86 architecture detection to both `install.sh` and `update_binary`, downloading official `tailcat_linux_amd64` binaries for x86-based Asuswrt-Merlin environments, QEMU, and container testbeds.
* **🩹 Self-Healing CLI Dependency Installation:** Invoking tunnel subcommands (`ssh`, `view`, `recv`, `files`, `webgui`) on systems missing the Go engine binary or view shell automatically downloads and sets up dependencies on-the-fly.
* **🧹 Self-Healing Dead Process & Watchdog Cleanup:** Enhanced `is_service_active` to detect externally terminated processes, auto-remove stale `.env` files and address dumps, and terminate orphaned watchdog subshells.
* **🌐 Accurate Protocol & Port Extraction:** Unified `get_webgui_connect_info` across multi-session overview, single-session card, and `tailcatzero status` for seamless HTTP (port 80) and HTTPS (custom ports / 8443) WebGUI access.
* **🗑️ Comprehensive Uninstaller Hardening:** Added removal of legacy `/opt/bin/tailcat` symlinks, address files, and POSIX case-insensitive `/jffs/scripts/init-start` cleanup.

---

## [1.4.0] - 2026-09-03

### 🚀 Rebranding to TAILCAT ZER0, Non-Interactive CLI Interface (`tailcatzero`), Persistent Mode & Dual Self-Updater

* **🐱 Project Rebranding to TAILCAT ZER0:** Officially rebranded to **TAILCAT ZER0** with the dedicated CLI command `tailcatzero`.
* **🖥️ Dedicated Non-Interactive CLI Dispatcher:** Full command-line interface accessible via `tailcatzero` across `/jffs/scripts/` and `/opt/bin/`. Supports subcommands (`status`, `stop [all|SVC]`, `ssh`, `webgui`, `update`, `-v`, `-h`) for headless automation and non-interactive SSH sessions.
* **⏱️ Persistent Mode & Custom Timeout:** Users can specify any custom auto-kill duration in minutes or enter `0` (or `persistent`) for persistent tunnels that run until manually stopped without spawning background sleep watchdog processes.
* **🔁 Interactive In-Place Input Validation:** Timeout configuration now features an interactive retry loop that reprompts on non-digit or invalid input without dropping the user back to the main menu.
* **🔄 Dual Script & Engine Self-Updater:** Option 6 (`manage_tailcat_menu`) and `tailcatzero update` fetch both the latest CLI script from GitHub (`underd0se/tailcat-zero`) and the official Go engine binary for the router architecture.
* **🔑 Guaranteed Ephemeral Tokens (`--key=new`):** Enforces `--key=new` across all tunnel spawns so every session generates a fresh, unique cryptographic WireGuard keypair and address token, preventing token reuse.
* **📋 Streamlined 6-Item Menu:** Consolidated configuration, update, reinstall, and uninstall options into a dedicated management submenu with active status badges.

---

## [1.3.0] - 2026-09-02

### 🚀 Multi-Service Concurrency, Side-by-Side ASCII Cat Header & Interactive TUI Management

* **⚡ Multi-Service Concurrent Management:** Full concurrent execution and management across all 4 services (Remote Shell, File DropBox, SFTP Directory Share, and Router WebGUI). Multiple tunnels can run simultaneously with independent WireGuard userspace nodes, tokens, and watchdog auto-kill timers in `/tmp/tailcat_sessions/`.
* **🐱 Side-by-Side ASCII Cat Header:** Compact, elegant side-by-side Japanese ASCII cat (`╱|、`) and project title/description layout.
* **⚙️ Dedicated Management Submenu:** Interactive management menu allowing users to update the Go engine binary, perform a fresh reinstall, or execute a complete uninstallation with clean init hook removal.
* **🛑 Selective & Batch Process Killer:** Stop action (`S`) lists all running services with PIDs and remaining time, allowing users to stop specific individual processes or all tunnels simultaneously.
* **✨ Flicker-Free Clean Canvas & Toast Banners:** Integrated VT100 screen-clearing (`clear_screen`) and transient `FLASH_MSG` toast banners across all menus and cancellations to prevent dirty terminal scrolling.
* **🔤 Natural Underlined Hotkey Styling:** Clean single-character underlined hotkeys (`<u>V</u>iew Sessions | 🛑 <u>S</u>top | ↩️ <u>E</u>xit: `) without redundant `=` symbols.

---

## [1.2.0] - 2026-09-02

### 🚀 TUI Dashboard, Dynamic Storage Awareness & Smart Status Badges

* **📺 Flicker-Free Live Dashboard:** Screen cleanly refreshes without terminal history scroll clutter when updating live timers (`r`) or toggling ASCII QR codes (`q`).
* **💾 Dynamic DropBox Storage Awareness:** Displays real-time free disk space for volatile RAM (`/tmp`) and mounted USB partitions (`/tmp/mnt/*`), with dynamic choice numbering based on USB presence.
* **🌐 Browser-Ready WebGUI Guidance:** Generates step-by-step instructions for remote administrators connecting via SOCKS5 proxy (`1. Run: tailcat socks <token>`, `2. Open browser: https://localhost:8443`).
* **🟢 Smart Status Indicators & Menu Badging:** Added prominent visual status badge (`🟢 ACTIVE` vs `⚪ INACTIVE`) and dynamically badges menu options 5 & 6 with active timer and kill labels.
* **⏱️ Precision Auto-Kill Warning:** Displays `< 1m remaining (expiring soon)` when session timer falls below 60 seconds.

---

## [1.1.3] - 2026-09-02

### ↩️ Submenu Navigation & Cancellation Support

* **↩️ Submenu Cancel Navigation:** Added full support for canceling and returning to the main menu using `e` / `b` / `cancel` from DropBox destination selection, SFTP directory/mode prompts, and Auto-Kill timeout configuration, preventing accidental tunnel launches.

---

## [1.1.2] - 2026-09-02

### 🧹 UI Cleanup, Anti-Bleed Dividers & Underline Hotkeys

* **🧹 Active Session Header:** Renamed card header to concise `Active Session`.
* **✂️ Cleaner Status Details:** Removed redundant explanations from Auto-Kill and Security metadata rows.
* **🛡️ Anti-Bleed Snippet Dividers:** Replaced fixed-width closed boxes with horizontal rule dividers (`───`) so long tokens and commands naturally flow without line-wrap border corruption.
* **📁 Folder Icon Alignment:** Updated SFTP Directory Sharing icon to `📁` (Folder) for improved visual metaphor.

---

## [1.1.1] - 2026-09-02

### 🎨 High-Contrast Terminal Color Refinements

* **🎨 Enhanced Dark-Theme Readability:** Upgraded Chat Invite Snippets to high-contrast crisp white (`C_WHITE`) and cyan borders (`C_CYAN`) with highlighted yellow commands (`C_YELLOW`), ensuring pristine visibility across dark-background terminals (Ghostty, iTerm2, macOS Terminal).

---

## [1.1.0] - 2026-09-02

### ✨ Modern UX, Live Session Card & KISS Architecture Refactor

* **📱 Live Active Session Card:** Dedicated interactive dashboard displaying live auto-kill countdown, active service details, and single-key actions (`[s] Stop`, `[r] Refresh`, `[q] QR Code`, `[b] Back`).
* **📷 Integrated ASCII QR Codes:** Direct rendering of ASCII QR codes in terminal via router's built-in `qrencode` for rapid mobile/tablet token capture.
* **💬 Ready-to-Paste Chat Snippets:** Generates pre-formatted 2-line invite text ready to copy-paste into Discord, Slack, or WhatsApp.
* **⚡ Global Contextual Hotkeys:** Single-key controls across the menu (`s` to stop immediately, `t` for timeout, `v` for session card).
* **🎯 KISS Feature Alignment:** Streamlined menu to the 4 core sharing pillars (Shell, DropBox, SFTP, WebGUI), eliminating unnecessary feature creep.
* **💾 Dynamic USB Storage Detection:** Automatically offers mounted USB partitions (`/tmp/mnt/*`) for DropBox storage to avoid RAM exhaustion.

---

## [1.0.0] - 2026-09-02

### 🚀 Initial Release: Ephemeral WireGuard Tunnel & DropBox Manager

* **🆘 Instant Remote Shell (Passwordless):** Ephemeral WireGuard shell powered by TailCat's native SSH server with capability-based token access (no passwords or SSH keys to configure).
* **📥 Encrypted File DropBox:** Write-only peer-to-peer file drop receiver into `/tmp/tailcat-inbox`.
* **📤 SFTP File Share:** Read-only directory serving with native SFTP path confinement.
* **🌐 WebGUI Remote Port Forwarder:** Securely forward local router management WebUI (port 8443).
* **⏱️ Automated 30-Minute Session Auto-Kill:** Background supervisor process automatically tears down active sessions when timer expires.
* **📦 Universal ARMv7 & ARM64 Architecture Support:** Automatically fetches official `tailscale/tailcat` `v0.4.0` static binaries for all Asuswrt-Merlin routers.
