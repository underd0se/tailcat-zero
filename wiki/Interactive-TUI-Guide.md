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

When sessions are active and a guest submits an on-demand permission request, the dashboard dynamically alerts you with a notification badge:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🔔 [1 Permission Request(s) Pending — Press P to Review]

  1. 🆘 Remote Support Shell         Full root or view-only access      [🔒 View: 24m]
  2. 📥 Receive Files                Direct P2P file transfer           [⚪ Inactive]
  3. 📁 Share Directory (SFTP)       Share a folder from your drive     [🟢 Active: 24m]
  4. 🌐 Expose Router WebGUI         Access to router's web interface   [⚪ Inactive]

  ------------------------------------------------------------------------

  5. ⏱️ Configure Default Timeout    Current: 30 min
  6. ⚙️ Manage TAILCAT ZER0          Update, reinstall, or remove

========================================================================

  🔔 Pending (1)  |  👁️ View Sessions  |  🛑 Stop  |  ↩️ Exit: 
```

### Dynamic Service Status Indicators
* `[⚪ Inactive]`: Service is not currently running.
* `[🟢 Active: Xm]`: Service is live, showing minutes remaining before auto-kill teardown.
* `[🔒 View: Xm]`: View-only diagnostic shell is running.
* `[🟢 Root + 🔒 View]`: Both root and view-only remote support shells are running concurrently.
* `🔔 [1 Permission Request(s) Pending]`: Displayed whenever a guest connected to a view-only session has submitted an on-demand command execution request. Pressing `P` directly opens the interactive approval modal.

---

## 📱 The Active Session Card

When you launch any service or press `v` (View Sessions), TAILCAT ZER0 renders a dedicated **Active Session Card**:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🐱 TAILCAT ZER0 — Active Session
========================================================================

  Service:     🔒 View-Only Diagnostic Shell
  Auto-Kill:   ⏱️ 28m remaining
  Security:    🔒 WireGuard P2P • Read-Only (System Writes Blocked)

  💬 Copy & Paste to Friend / Admin Support:
  ────────────────────────────────────────────────────────────────────────
  Hey, I've opened a temporary read-only diagnostic TailCat session on my router (28m remaining).
  Run: tailcat ssh tcpGFwWCBquiVYgLrL7k3HQDl_jERoGKCT7I5VYUIQeZ6LF_q4e2FrWCCGx_dEaW2LF0g2e2n6b1rBPDOYeDTyCVNS2qKtROJmKmFxWCCufT1iqPy9rsILOQQ7EUZk6HsfbXwb3xyO8RbF99tPgGFpGQEv
  ────────────────────────────────────────────────────────────────────────

========================================================================

  🛑 Stop  |  📱 QR Code  |  🔄 Refresh  |  ↩️ Back: 
```

### Hotkey Actions on Session Cards:
| Key / Number | Action | Description |
|---|---|---|
| `s` / `1` | **Stop Session** | Safely terminates the tunnel and revokes the capability token immediately. |
| `q` | **QR Code** | Toggles an inline ASCII QR code of the capability token. |
| `r` | **Refresh** | Refreshes the countdown timer and checks connection state. |
| `b` / `Enter` | **Back** | Returns to the dashboard (leaves tunnel running in background). |

---

## 🗂️ Multi-Session Overview

When two or more services run simultaneously, pressing `v` (View Sessions) opens the **Multi-Session Overview**:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🐱 TAILCAT ZER0 — Active Sessions (2 Running)
========================================================================

  [1] 🔒 View-Only Diagnostic Shell        ⏱️ 26m remaining  🔒 P2P
  [2] 📁 SFTP File Share (/jffs)           ⏱️ 26m remaining  🔒 P2P

  💬 Copy & Paste Connect Commands:
  ────────────────────────────────────────────────────────────────────────
  • VIEW:    tailcat ssh tcpGFwWCBquiVYgLrL7k3HQDl...
  • FILES:   tailcat ls -l tcpGFwWCBwoiH7ENLsCVJqJ...
  ────────────────────────────────────────────────────────────────────────

========================================================================

  👁️ View (1-2)  |  🛑 Stop All  |  🔄 Refresh  |  ↩️ Back: 
```

From this overview:
* Type `1` or `v1` to open the dedicated card for session 1.
* Type `2` or `v2` to open session 2.
* Type `s` to stop all sessions.

---

## 🛑 Safe Stop Confirmation

Pressing `s` (Stop) prompts you to confirm which session to terminate, preventing accidental service interruptions:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🛑 Active Sessions to Stop:

  1. 🔒 View-Only Diagnostic Shell
  2. 📁 SFTP File Share (/jffs)

========================================================================

  🛑 Stop (1-2)  |  🛑 All  |  ↩️ Back: 
```

* Enter `1` or `s` to stop session 1.
* Enter `2` to stop session 2.
* Enter `a` to stop all sessions.
* Enter `b` to cancel.

---

## 📷 Inline ASCII QR Codes

If Entware has `qrencode` installed, pressing `q` renders an inline ASCII QR code directly inside your terminal:

```text
█████████████████████████████████████████████████
█████████████████████████████████████████████████
████ ▄▄▄▄▄ ██▀▀ █ ▄▄▄▄▄ ████████ ▄▄▄▄▄ ██▀▀ █ ███
████ █   █ █ █ ▄█ █   █ ████████ █   █ █ █ ▄█ ███
████ █▄▄▄█ █▀ █▄█ █▄▄▄█ ████████ █▄▄▄█ █▀ █▄█ ███
████▄▄▄▄▄▄▄█▄█ █▄█▄▄▄▄▄▄▄████████▄▄▄▄▄▄▄█▄█ █▄███
...
```

This allows instant token capture using mobile devices or laptops with webcams without copying long tokens.

---

## ⏱️ Timeout Management & Persistent Mode

By default, every session is guarded by an ephemeral watchdog timer set to **30 minutes**.

Select **Option 5 (Configure Default Timeout)** from the main menu:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  ⏱️ Configure Session Auto-Kill Timeout

  Current Timeout: 30 minutes

  1. ⏱️ 30 Minutes                   Standard auto-kill (Default)
  2. ⏱️ 60 Minutes                   1 Hour session
  3. ⏱️ 120 Minutes                  2 Hours session
  4. ♾️  Persistent (0 min)           No auto-kill; runs until stopped

========================================================================

  ⏱️ Set Timeout (1-4 or minutes)  |  ↩️ Back: 
```

* Enter `1`, `2`, or `3` for quick presets.
* Enter custom minutes (e.g. `45` or `180`).
* Enter `4` or `p` for **Persistent Mode** (`0 min`), disabling auto-kill until manually stopped.

---

## ⚙️ Managing TAILCAT ZER0

Select **Option 6 (Manage TAILCAT ZER0)** from the main menu:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  ⚙️  Manage TAILCAT ZER0:

  1. 🔄 Check & Update TAILCAT ZER0  Update script & engine (v1.7.1 / v0.6.0)
  2. ⚡ Force Reinstall TAILCAT ZER0 Fresh download of script & binary
  3. 🗑️ Complete Uninstall           Remove addon, configs & init hooks

========================================================================

  🔄 Update  |  ⚡ Reinstall  |  🗑️ Complete Uninstall  |  ↩️ Back: 
```

* Enter `u` or `1` to run the smart updater.
* Enter `r` or `2` to force-reinstall fresh copies from GitHub.
* Enter `c` or `3` to perform a complete clean uninstallation.
