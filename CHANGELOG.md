# Changelog ─ TailCat-Merlin

All notable changes to TailCat-Merlin are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
