# 🐱 TailCat-Merlin

> **Ephemeral WireGuard Tunnels, Remote Support Shells & Encrypted DropBoxes for Asuswrt-Merlin Routers**

Powered by [Tailscale's TailCat](https://github.com/tailscale/tailcat) engine (`magicsock` + WireGuard + DERP NAT traversal) without requiring a Tailscale account or central coordination server.

---

```text
========================================================================
  TailCat-Merlin v1.0.0 — Ephemeral WireGuard Tunnel Manager
  Status: [ ACTIVE: SSH | 26m remaining ]
========================================================================
  1. 🆘 Start Remote SSH Tunnel      (Port 22, Password/Key Required)
  2. 📥 Receive Files / DropBox      (Drop to /tmp/tailcat-inbox)
  3. 📤 Serve Directory (SFTP)       (Read-Only File Sharing)
  4. 🌐 Expose Router WebGUI         (Port 8443 Tunnel)
  5. 🔌 Custom Port Forwarder        (Any TCP Port or Range)
  6. 🛡️ Ephemeral Exit Node          (Route client traffic via router)
  7. 👁️ View Active Session Token    (Display connect command)
  8. 🛑 Stop Active Session          (Kill active tunnel)
  ------------------------------------------------------------------------
  9. ⏱️ Configure Auto-Kill Timeout  (Current: 30 min)
 10. 🔄 Reinstall / Update Binary    (Download latest v0.4.0)
 11. 🗑️ Uninstall TailCat Addon     (Clean up all files)
========================================================================
 Enter selection [1-11, e=Exit]: 
```

---

## ⚡ Quick Install

Run this command directly in your router SSH terminal:

```sh
curl -fsSL https://raw.githubusercontent.com/underd0se/tailcat-merlin/main/install.sh | sh
```

To launch the manager at any time:
```sh
tailcat
```

---

## 🌟 Key Features

* **🆘 Emergency Remote SSH:**
  * Spawns an encrypted P2P WireGuard tunnel directly to your router's Dropbear SSH port (22).
  * **Strictly Protected:** Requires your router's standard SSH password or public key (no unauthenticated root backdoors).
* **📥 Encrypted File DropBox:**
  * Turn your router into a secure drop box (`/tmp/tailcat-inbox`).
  * Send firmware images or JFFS backups from any PC: `tailcat cp backup.tar.gz <token>:`
* **📤 SFTP Directory Share:**
  * Serve any router directory (e.g. `/jffs` or USB mount) read-only to remote clients using native SFTP.
* **🌐 WebGUI Remote Access:**
  * Expose local WebUI (port 8443) over a secure token without opening WAN firewall ports.
* **⏱️ Built-In Auto-Kill Supervisor:**
  * All tunnels automatically expire and kill themselves after a configurable timer (default: **30 minutes**).
* **amtm Integration:**
  * Fully registered as an `amtm` menu addon.

---

## 🔒 Security Architecture

| Security Measure | Implementation |
|---|---|
| **No WAN Ports Open** | Uses DERP relays and UDP NAT hole-punching. Zero incoming firewall holes opened. |
| **Enforced Authentication** | Remote SSH proxies to local port 22, enforcing standard router credentials. |
| **Mandatory Auto-Kill** | Background watchdog subshell automatically kills tunnels when the timer expires. |
| **Clean Reboot Teardown** | Session locks and state are stored in volatile memory (`/tmp`) and cleaned up on reboot. |

---

## 💻 Connecting from Client Machines

Install `tailcat` on your laptop or client device:

* **macOS:** `brew install tailcat`
* **Linux / Go:** `go install github.com/tailscale/tailcat/cmd/tailcat@latest`
* **Prebuilt Binaries:** [TailCat Releases](https://github.com/tailscale/tailcat/releases)

### Client Examples:

```sh
# Connect to router SSH
tailcat ssh tcXXXXXXXXX

# Send file to router
tailcat cp firmware.trx tcXXXXXXXXX:

# Browse router files
tailcat ls tcXXXXXXXXX
```

---

## 🗑️ Uninstallation

Launch `tailcat` and select **Option 11**, or run:
```sh
rm -rf /jffs/addons/tailcat /jffs/scripts/tailcat /jffs/addons/amtm/tailcat.mod
```

---

## 📜 License

GPL-3.0 License.
