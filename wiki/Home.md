# 🐱 TAILCAT ZER0 Documentation

Welcome to the official **TAILCAT ZER0** Wiki.

> **Ephemeral WireGuard Tunnels, Remote Support Shells & Encrypted File Inboxes for Asuswrt-Merlin Routers**

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7  
                                  |、˜〵          
  Instant Tunnel Manager         じしˍ,)ノ
```

---

## 🌟 What is TAILCAT ZER0?

**TAILCAT ZER0** is a secure, ephemeral remote access and diagnostic management suite built natively for **Asuswrt-Merlin** routers. Powered under the hood by [Tailscale's TailCat](https://github.com/tailscale/tailcat) engine (`magicsock` + WireGuard + DERP NAT traversal), it delivers peer-to-peer, encrypted connectivity without requiring a central coordination server, tailnet, or Tailscale account.

Whether you need to let a trusted forum helper diagnose a routing issue, securely transfer firmware backups from your laptop, or inspect your router's WebGUI from behind a double-NAT CGNAT connection, TAILCAT ZER0 provides instant capabilities with zero open firewall ports.

---

## 🚀 Core Value Propositions

* 🔒 **Zero Disclosed Passwords / Zero Keys to Manage:** Connections are authenticated using single-use 256-bit cryptographically secure capability tokens. Possession is permission.
* 🛡️ **Zero-Trust View-Only Diagnostic Shell:** Let remote technicians or forum helpers inspect logs, routes, interfaces, and NVRAM variables in an aggressively restricted sandbox where destructive actions, file modifications, subshells, and sensitive credential files are completely blocked.
* 🔔 **Live On-Demand Permission Escalation:** Need a guest to run a specific command outside the default sandbox? They run `request <cmd>`, and you can review and approve it directly from your terminal or TUI with a single keystroke.
* 🌐 **Zero WAN Ports Opened:** Penetrates carrier-grade NATs (CGNAT), cellular modems, and corporate firewalls seamlessly using Tailscale's global DERP relay fabric and STUN UDP hole-punching.
* ⏱️ **Automatic Ephemeral Teardown:** Every session has an automatic watchdog countdown (default: 30 minutes) or can run persistently until manually stopped. Volatile memory locks ensure zero remnants survive a router reboot.
* 🖐️ **5-Slot Multi-Service Concurrency:** Run root shells, view-only triage sessions, File Receivers, SFTP shares, and WebGUI proxies concurrently on independent WireGuard nodes.

---

## 📚 Wiki Navigation Index

Explore the detailed documentation chapters:

| Chapter | Description |
|---|---|
| **[Installation & Getting Started](Installation-&-Getting-Started)** | Hardware compatibility, prerequisites, one-line installation, updates, and uninstallation. |
| **[Interactive TUI Guide](Interactive-TUI-Guide)** | Full walkthrough of the retro cyberpunk terminal interface, session cards, ASCII QR codes, and chat snippets. |
| **[Remote Support Shells](Remote-Support-Shells)** | Choosing between Root Shells and Restricted View-Only Shells, connection procedures, and use cases. |
| **[View-Only Sandbox & Escalation](View-Only-Sandbox-&-Permission-Escalation)** | Zero-trust sandbox architecture, built-in commands, blocked syntax, GTFOBin defenses, Hard Red Lines, and on-demand escalation. |
| **[P2P File Transfers & Inbox](File-Transfers-&-Inbox)** | Direct router file receiver inboxes for firmware/backups, SFTP directory sharing (read-only and read-write). |
| **[WebGUI Remote Access](WebGUI-Remote-Access)** | Exposing Asuswrt WebUI securely over WireGuard with zero-config client port forwarding. |
| **[CLI Reference & Automation](CLI-Reference-&-Headless-Automation)** | Complete command-line syntax, scripting integration, headless approvals, and cron usage. |
| **[Security Architecture](Security-Architecture)** | Cryptographic model, WireGuard/Noise protocol, volatile locks, DERP routing, and threat model. |
| **[Troubleshooting & FAQ](Troubleshooting-&-FAQ)** | Diagnostic checklist, common issues, DERP relay checks, Entware setup, and forum helper guide. |

---

## ⚡ Quick Start

Run the one-line installer directly in your router's SSH terminal:

```sh
curl -fsSL https://raw.githubusercontent.com/underd0se/tailcat-zero/main/install.sh | sh
```

Launch the interactive dashboard:

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

Or spin up an instant view-only diagnostic shell for remote help:

```sh
tailcatzero view
```
