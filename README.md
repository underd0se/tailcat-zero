# 🐱 TAILCAT ZER0

> **Ephemeral WireGuard Tunnels, Remote Support Shells & Encrypted File Inboxes for Asuswrt-Merlin Routers**

[![Release](https://img.shields.io/badge/version-v1.11.1-blue.svg)](CHANGELOG.md)
[![Firmware](https://img.shields.io/badge/Asuswrt--Merlin-384.13%2B-orange.svg)](https://www.asuswrt-merlin.net/)
[![License](https://img.shields.io/badge/license-GPL--3.0-green.svg)](LICENSE)
[![Engine](https://img.shields.io/badge/powered%20by-Tailscale%20TailCat-blueviolet.svg)](https://github.com/tailscale/tailcat)

Powered by [Tailscale's TailCat](https://github.com/tailscale/tailcat) engine (`magicsock` + WireGuard + DERP NAT traversal) without requiring a Tailscale account, central coordination server, or public open ports.

---

```text
  TAILCAT ZER0 v1.11.1             ╱|、
                                 (˚ˎ 。7  
                                  |、˜〵          
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  1. 🆘 Remote Support Shell         View-only (safe) or full root     [🟢 Root + 🔒 View]
  2. 📥 Receive Files & Folders      Direct P2P file/folder drop box   [⚪ Inactive]
  3. 📁 Share Directory (SFTP)       Share a folder from your drive    [🟢 Active: 28m]
  4. 🌐 Expose Router WebGUI         Access to router's web interface  [⚪ Inactive]

  ------------------------------------------------------------------------

  5. ⏱️ Configure Default Timeout    Current: 30 min
  6. ⚙️ Manage TAILCAT ZER0          Update, reinstall, or remove

========================================================================

  👁️ View Sessions  |  🛑 Stop  |  ↩️ Exit: 
```

---

## ⚡ Quick Install

Run this command in your router SSH terminal:

```sh
curl -fsSL https://raw.githubusercontent.com/underd0se/tailcat-zero/main/install.sh | sh
```

Launch the interactive TUI:
```sh
tailcatzero
```

---

## 🎯 What is TAILCAT ZER0?

* **Zero Accounts & Servers:** Generates standalone, capability-based 256-bit WireGuard tokens. No Tailscale account, API keys, or central coordination server needed.
* **Zero Open Ports:** Traverses CGNAT, double-NAT, and strict firewalls using WireGuard + DERP relay fallback without opening any WAN firewall ports.
* **Zero-Trust Security:** Drop-in connections default to an isolated, read-only C99 diagnostic sandbox (`tailcat-view-shell`). Full root shell requires deliberate confirmation.
* **Auto-Kill & Ephemeral:** Background watchdog automatically shuts down sessions after a configurable timeout (default: 30 mins) with broadcast countdown warnings at 5m and 1m.

---

## 🛠️ Core Capabilities

| Feature | Description | Default Mode |
|---|---|---|
| **🔒 View-Only Shell** | Safe diagnostic shell in C99 with Tab autocompletion & command history. System writes, deletions, and state changes are blocked. Real-time `request <cmd>` approval for elevated tools. | **Default** |
| **⚡ Full Root Shell** | Unrestricted administrative SSH terminal. Requires explicit `YES` confirmation. | Opt-in |
| **📥 File Receiver Inbox** | Secure P2P file receiver into volatile RAM (`/tmp/tailcat-inbox`) or mounted USB drives. | Ready |
| **📁 SFTP Directory Share** | Serve router paths (e.g. `/jffs` or USB drives) securely to remote SFTP clients. | Read-Only or R/W |
| **🌐 WebGUI Tunneling** | Forward router web interface (port 8443/80) securely over encrypted WireGuard tunnel. | Ready |

---

## 💻 Connecting from Client Machines

Install the official `tailcat` client on your computer:
* **macOS:** `brew install tailcat`
* **Linux / Go:** `go install github.com/tailscale/tailcat/cmd/tailcat@latest`
* **Binaries:** [TailCat Releases](https://github.com/tailscale/tailcat/releases)

### Quick Commands:
```sh
# Connect to Remote Support Shell (view-only or root)
tailcat ssh <token>

# Send a file to the router inbox
tailcat cp backup.tar.gz <token>:

# Browse SFTP shared files
tailcat ls <token>

# Forward WebGUI to your local browser (open https://localhost:8443)
tailcat forward <token> 8443
```

---

## 📚 Documentation & Guides

Detailed architectural guides, advanced options, and configuration references are available in the **[Official Wiki](wiki/Home.md)**:

* 🚀 **[Installation & Getting Started](wiki/Installation-&-Getting-Started.md)** — Requirements, installation, updating, and initial configuration.
* 🖥️ **[Interactive TUI Guide](wiki/Interactive-TUI-Guide.md)** — Complete dashboard walkthrough, session cards, QR codes, and hotkeys.
* 🔒 **[View-Only Sandbox & Escalation](wiki/View-Only-Sandbox-&-Permission-Escalation.md)** — C99 sandbox security, allowed commands, and in-band permission approval.
* 🆘 **[Remote Support Shells](wiki/Remote-Support-Shells.md)** — View-only vs root shells, helper instructions, and zero-trust safeguards.
* 📥 **[File Transfers & Inbox](wiki/File-Transfers-&-Inbox.md)** — P2P file drop receiver, USB storage, and SFTP sharing.
* 🌐 **[WebGUI Remote Access](wiki/WebGUI-Remote-Access.md)** — Web management forwarding, SSL/HSTS browser setup, and local port binding.
* ⚡ **[CLI Reference & Headless Automation](wiki/CLI-Reference-&-Headless-Automation.md)** — Scriptable commands (`status`, `ssh`, `webgui`, `stop`, `timeout`).
* 🛡️ **[Security Architecture](wiki/Security-Architecture.md)** — Threat model, PID rollover safeguards, flash wear elimination, and encryption.
* ❓ **[Troubleshooting & FAQ](wiki/Troubleshooting-&-FAQ.md)** — Common questions, connection diagnostics, and solutions.

---

## 🗑️ Uninstallation

From the TUI, select **Option 6 ➔ Option 3**, or run:
```sh
rm -rf /jffs/addons/tailcatzero /jffs/addons/tailcat /jffs/scripts/tailcatzero /jffs/scripts/tailcat /opt/bin/tailcatzero
```

---

## 📝 Changelog & License

* Complete version history and release notes: **[CHANGELOG.md](./CHANGELOG.md)**
* Licensed under the **[GPL-3.0 License](./LICENSE)**.
