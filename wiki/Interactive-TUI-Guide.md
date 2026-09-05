# 🖥️ Interactive TUI Guide

**TAILCAT ZER0** features a responsive, keyboard-driven terminal dashboard designed specifically for Asuswrt-Merlin SSH sessions. It provides real-time service indicators, countdown timers, inline ASCII QR codes, and quick copy-paste snippets.

---

## 🎨 Main Dashboard Layout

Run `tailcatzero` in your router shell to open the dashboard:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7  
                                  |、˜〵          
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  1. 🆘 Remote Support Shell         Full root or view-only access     [🟢 Root + 🔒 View]
  2. 📥 Receive Files                Direct P2P file transfer          [⚪ Inactive]
  3. 📁 Share Directory (SFTP)       Share a folder from your drive    [🟢 Active: 28m]
  4. 🌐 Expose Router WebGUI         Access to router's web interface  [⚪ Inactive]

  ------------------------------------------------------------------------

  5. ⏱️ Configure Default Timeout    Current: 30 min
  6. ⚙️ Manage TAILCAT ZER0          Update, reinstall, or remove

========================================================================

  [v] View Sessions  |  [s] Stop  |  [P] 🔔 1 Pending Request  |  [e] Exit: 
```

### Dynamic Service Status Indicators
* `[⚪ Inactive]`: Service is not currently running.
* `[🟢 Active: Xm]`: Service is live, showing minutes remaining before auto-kill teardown.
* `[🟢 Root + 🔒 View]`: Both root and view-only remote support shells are running concurrently.
* `[🔔 [P]ending Requests]`: Displayed whenever a guest connected to a view-only session has submitted an on-demand command execution request. Pressing `P` directly opens the interactive approval modal.

---

## 📱 The Active Session Card

When you start any service or press `v` (View Sessions), TAILCAT ZER0 renders a dedicated **Active Session Card**:

```text
========================================================================
  SESSION CARD: RESTRICTED VIEW-ONLY SHELL
========================================================================

  Service:           SSH View-Only Diagnostic Shell
  Status:            🟢 ACTIVE
  Auto-Kill In:      27 min (Auto-teardown watchdog active)
  Access Token:      tc8d2a1b9f7c04e3

------------------------------------------------------------------------
  HOW YOUR GUEST CONNECTS:
------------------------------------------------------------------------
  Run this on client machine (macOS / Linux / Windows):
  $ tailcat ssh tc8d2a1b9f7c04e3

------------------------------------------------------------------------
  READY-TO-PASTE INVITE (Copy & send to helper):
------------------------------------------------------------------------
  I've started a read-only diagnostic session on my router.
  Connect with: tailcat ssh tc8d2a1b9f7c04e3
========================================================================

  🛑 [s] Stop  |  📱 [q] QR Code  |  🔄 [r] Refresh  |  ↩️ [b] Back: 
```

### Hotkey Actions on Session Cards:
| Key | Action | Description |
|---|---|---|
| `s` | **Stop Session** | Prompts to safely terminate the selected tunnel and revoke the token immediately. |
| `r` | **Refresh** | Refreshes the countdown timer and checks connection state. |
| `q` | **QR Code** | Displays an inline ASCII QR code of the capability token. |
| `b` | **Back** | Returns to the main menu (leaves tunnel running in background). |

---

## 📷 Inline ASCII QR Codes

If Entware has `qrencode` installed, pressing `q` renders an inline ASCII QR code directly inside your terminal:

```text
█████████████████████████████
█████████████████████████████
████ ▄▄▄▄▄ ██▀▀ █ ▄▄▄▄▄ ████
████ █   █ █ █ ▄█ █   █ ████
████ █▄▄▄█ █▀ █▄█ █▄▄▄█ ████
████▄▄▄▄▄▄▄█▄█ █▄█▄▄▄▄▄▄▄████
...
```

This allows instant token capture using mobile devices running TailCat or smartphone cameras, without needing to manually transcribe or copy long capability tokens across devices.

---

## ⏱️ Timeout Management & Persistent Mode

By default, every session is guarded by an ephemeral watchdog timer set to **30 minutes**.

### Configuring Timeout:
1. Select **Option 5 (Configure Default Timeout)** from the main menu.
2. Enter the desired duration in minutes (e.g., `15`, `60`, `120`).
3. **Persistent Mode:** Enter `0` or `persistent`. When set to `0`, tunnels run indefinitely until explicitly stopped via the TUI, CLI (`tailcatzero stop`), or router reboot.

---

## 🖐️ 5-Slot Multi-Service Concurrency

TAILCAT ZER0 supports up to **5 concurrent services** running at the same time:
1. `SSH_ROOT`: Full administrative root shell.
2. `SSH_VIEW`: Restricted read-only diagnostic shell.
3. `RECV`: Encrypted drop box receiver.
4. `FILES`: SFTP directory share.
5. `WEBGUI`: Encrypted WebGUI proxy tunnel.

Each service runs on its own isolated ephemeral WireGuard node with its own unique capability token, meaning you can share view-only access with a forum helper while simultaneously receiving a firmware backup from your desktop PC without any token conflicts or crosstalk.
