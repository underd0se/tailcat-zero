# Changelog ─ TailCat-Merlin

All notable changes to TailCat-Merlin are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
