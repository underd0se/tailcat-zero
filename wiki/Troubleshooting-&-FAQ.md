# ❓ Troubleshooting & FAQ

This guide provides troubleshooting solutions for common issues, answers frequently asked questions, and offers tips for Asuswrt-Merlin community members and forum helpers.

---

## 🔍 Diagnostic Checklist

If you encounter an issue running TAILCAT ZER0, run through this quick checklist:

### 1. Check JFFS Partition
Ensure `/jffs` is mounted and has sufficient free disk space:
```sh
df -h /jffs
```
*Requirement: At least 15MB of free space.*

### 2. Verify Internet & DNS Connectivity
Confirm your router can resolve domain names and establish outbound connections to GitHub and DERP relays:
```sh
curl -sI https://github.com | head -n 1
ping -c 3 1.1.1.1
```

### 3. Verify TailCat Engine Execution
Test whether the compiled engine binary runs without missing libraries or architecture mismatch:
```sh
/jffs/addons/tailcatzero/bin/tailcat --version
```
*Expected: `tailcat v0.6.0` (or newer).*

---

## 🛠️ Common Issues & Fixes

### 1. "Engine binary missing or cannot execute"
* **Symptom:** `install.sh` or `tailcatzero` reports: `[✗] TailCat engine binary failed to execute`.
* **Cause:** The wrong CPU architecture was detected or the downloaded binary was truncated due to a transient network timeout.
* **Fix:**
  1. Check your router's architecture: `uname -m`
  2. Run the updater to fetch a fresh binary:
     ```sh
     tailcatzero update
     ```

---

### 2. "Connection timed out connecting to token from client"
* **Symptom:** Running `tailcat ssh <TOKEN>` on client hangs or prints `connection timed out`.
* **Causes:**
  * The session timer expired and the tunnel was automatically torn down.
  * Your client ISP blocks outbound UDP STUN packets and DERP TLS (port 443) fallback failed.
* **Fix:**
  1. On the router, verify the session is still active:
     ```sh
     tailcatzero status
     ```
  2. If the session expired, restart it: `tailcatzero view`.
  3. Ensure your client device is running the latest `tailcat` client:
     ```sh
     brew upgrade tailcat
     # or
     go install github.com/tailscale/tailcat/cmd/tailcat@latest
     ```

---

### 3. "Command restricted in view-only mode"
* **Symptom:** Guest types a command in view-only mode and receives:
  `[-] Command '<cmd>' is restricted in view-only mode.`
* **Cause:** The command is not part of the default read-only inspection allowlist.
* **Fix:**
  * Guest can type `request <cmd>` (or reply `y` when prompted).
  * Host router administrator approves the command via TUI (`P` hotkey) or CLI (`tailcatzero approve <id>`).

---

### 4. "QR Code is not displaying"
* **Symptom:** Pressing `q` in the session card shows a message that `qrencode` is not installed.
* **Cause:** The router does not have Entware's `qrencode` package installed.
* **Fix:**
  If you have Entware installed, simply run:
  ```sh
  opkg update && opkg install qrencode
  ```

---

### 5. "WebGUI shows SSL certificate security warning"
* **Symptom:** Browsing `https://localhost:8443` displays a browser warning: *Your connection is not private*.
* **Explanation:** Asuswrt-Merlin uses a self-signed SSL certificate by default. This is completely expected behavior.
* **Fix:** Click **Advanced ➔ Proceed to localhost (unsafe)** to open the login page. The connection remains fully encrypted through the WireGuard tunnel.

---

### 6. "Service is already running prompt"
* **Symptom:** Launching a service from the menu displays:
  `[!] Service 🆘 Remote Support Shell (Root) is already running (PID: 14205).`
* **Explanation:** TAILCAT ZER0 detects that a background instance of this service is already active on the router:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  [!] Service 🆘 Remote Support Shell (Root) is already running (PID: 14205).

  1. 👁️ View session card and token
  2. 🛑 Stop active session
  3. 🔄 Restart session                 Stop and launch with fresh token

========================================================================

  👁️ View (Default)  |  🛑 Stop  |  🔄 Restart  |  ↩️ Back: 
```

* **Actions:**
  * Press `v` or `1` (or `Enter`) to view the active session card and copy its token.
  * Press `s` or `2` to stop the running session immediately.
  * Press `r` or `3` to stop and cleanly restart the service with a fresh token.
  * Press `b` to cancel and return to the main dashboard.

---

## 💬 Frequently Asked Questions (FAQ)

### Q: Do I need a Tailscale account or credit card?
**A:** No. TAILCAT ZER0 uses Tailscale's open-source `tailcat` engine. It connects directly peer-to-peer using DERP relays for rendezvous without needing any user account, email address, or credit card.

### Q: Does TAILCAT ZER0 replace Asuswrt-Merlin's built-in WireGuard or OpenVPN servers?
**A:** No. TAILCAT ZER0 is designed for **ephemeral, on-demand management, file transfers, and remote assistance**. It runs independently alongside your existing VPNs, WireGuard tunnels, and VPN Director routing without interfering with them.

### Q: Does running TAILCAT ZER0 consume high CPU or RAM?
**A:** No. TailCat is written in Go and optimized for embedded Linux devices. When idle or routing small management sessions, it consumes minimal CPU (<1%) and roughly 15-25MB of RAM.

### Q: Can I run multiple services at the same time?
**A:** Yes! TAILCAT ZER0 supports up to **5 concurrent services** running in parallel (Root Shell, View Shell, DropBox, SFTP, and WebGUI). Each service receives its own isolated WireGuard node and unique capability token.

---

## 🤝 SNBForums & Helper Guide

If you help users troubleshoot their routers on forums like [SNBForums](https://www.snbforums.com):

### Quick Snippet to Send to a User:
> "To help diagnose this issue, please run this on your router:
> ```sh
> curl -fsSL https://raw.githubusercontent.com/underd0se/tailcat-zero/main/install.sh | sh
> tailcatzero view
> ```
> Then PM me the connection token shown on your screen (e.g. `tcXXXXXXXXXXXX`). This gives me read-only diagnostic access (I cannot edit files, reboot, or change settings), and it will automatically shut itself down after 30 minutes."

Once connected, you can inspect:
```sh
sysinfo
logread
ip route
nvram get wan0_ipaddr
netstat -tulpn
```
If you need to run an extra command, type `request <command>` and the user can approve it on their screen with one key.
