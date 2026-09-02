# Changelog ─ TailCat-Merlin

All notable changes to TailCat-Merlin are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.3.0] - 2026-09-02

### 🚀 Multi-Service Concurrency & Aesthetic ASCII Cat Branding

* **⚡ Multi-Service Concurrent Management:** Full concurrent execution and management across all 4 services (Remote Shell, File DropBox, SFTP Directory Share, and Router WebGUI). Multiple tunnels can run simultaneously with independent WireGuard userspace nodes, tokens, and watchdog auto-kill timers in `/tmp/tailcat_sessions/`.
* **📋 Multi-Service Active Dashboard:** Overview card displaying all running services, individual connect commands, copy-paste snippets, and fast 1-key switching/stopping.
* **🐱 Aesthetic Minimalist ASCII Cat Header:** Added a sleek, minimalist Japanese ASCII cat (`╱|、`) header in high-contrast cyan.
* **🧹 Clean Menu Typography:** Stripped outer parentheses from descriptions for streamlined column readability.
* **📐 Unified Bottom Action Bars:** Aligned the Main Menu bottom prompt (`Actions: 1-9 Menu Selection | 🛑 Stop | ⏱️ Timeout | ↩️ Exit:`) with the Active Session card format.

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
* **🛠️ amtm Integration:** Fully registered into the `amtm` menu hierarchy.
