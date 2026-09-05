# ⚙️ CLI Reference & Headless Automation

While TAILCAT ZER0 features a full interactive TUI, all features are exposed via non-interactive command-line subcommands. This allows direct scripting, headless SSH invocation, cron jobs, and integration with other Asuswrt-Merlin tools.

---

## 📖 Command Reference

Run `tailcatzero -h` or `tailcatzero --help`:

```text
TAILCAT ZER0 v1.8.0 — Ephemeral WireGuard Tunnel & Multi-Service Manager

Usage:
  tailcatzero                           Launch interactive TUI dashboard (default)
  tailcatzero status                    Display running sessions and connect tokens
  tailcatzero stop [all|SVC]            Stop all sessions or a specific service (SSH, VIEW, RECV, FILES, WEBGUI)
  tailcatzero ssh [root|view]           Start remote shell tunnel (default root)
  tailcatzero view                      Start restricted view-only diagnostic shell
  tailcatzero recv [dir]                Start encrypted file/folder receiver tunnel (default /tmp/tailcat-inbox)
  tailcatzero files [dir] [ro|rw]       Start SFTP directory share (default /jffs ro)
  tailcatzero webgui                    Start router WebGUI tunnel
  tailcatzero requests                  List pending guest permission requests
  tailcatzero approve [id|cmd] [--once] Approve guest request (session-wide or single-use)
  tailcatzero deny [id|cmd]             Deny guest request and suppress repeat prompts
  tailcatzero allow <cmd>               Proactively permit command in view-only mode
  tailcatzero revoke <cmd>              Revoke command permission from view-only mode
  tailcatzero timeout [min|persistent]  Get or configure default auto-kill session timeout
  tailcatzero update                    Update TAILCAT ZER0 script & engine (hash-verified)
  tailcatzero check-update              Check upstream version & hash for updates
  tailcatzero -v, --version             Show version & script hash
  tailcatzero -h, --help                Show this help message
```

| Command | Arguments | Description |
|---|---|---|
| `status` | *(none)* | Display running sessions, service type, process ID, capability token, and remaining timeout countdown. |
| `ssh` | `[root \| view]` | Start an ephemeral remote shell tunnel. Defaults to `root` if no argument is provided. |
| `view` | *(none)* | Shortcut to start a restricted view-only diagnostic shell session directly. |
| `recv` | `[/path/to/inbox]` | Start encrypted file and directory receiver inbox (`--accept-dirs`). Defaults to `/tmp/tailcat-inbox` if omitted. |
| `files` | `[/path] [ro \| rw]` | Start SFTP directory share. Defaults to `/jffs` with `ro` (read-only) mode if omitted. |
| `webgui` | `[port]` | Start Asuswrt WebGUI proxy tunnel. Defaults to HTTPS port `8443`. |
| `requests` | *(none)* | List all pending guest command permission escalation requests. |
| `approve` | `<id \| cmd> [--once]` | Approve a guest request. Appends `--once` for single-use execution; otherwise approves session-wide. |
| `deny` | `<id \| cmd>` | Deny a pending guest request and suppress repeat alerts. |
| `allow` | `<cmd>` | Proactively add `<cmd>` to the current view-only session allowlist. |
| `revoke` | `<cmd>` | Revoke permission for `<cmd>` from the current view-only session allowlist. |
| `timeout` | `[min \| persistent]` | Query current default timeout or set a new duration (in minutes, or `0` / `persistent`). |
| `stop` | `all` | Stop all active tunnels and terminate all background watchdogs. |
| `stop` | `<SERVICE>` | Stop a specific service (`SSH`, `VIEW`, `RECV`, `FILES`, or `WEBGUI`). |
| `update` | *(none)* | Check GitHub upstream for updates by cryptographic hash (`md5sum`) and upgrade script and engine if changed. |
| `check-update` | *(none)* | Non-destructively check whether upstream code has changed by comparing script hash and SemVer. |
| `-v`, `--version` | *(none)* | Display TAILCAT ZER0 version, active script hash, and installed engine binary version. |

---

## 💻 CLI Examples

### 1. Starting Services Non-Interactively
```sh
# Start a view-only shell for a forum helper
tailcatzero view

# Start an SFTP share of USB drive in read-only mode
tailcatzero files /tmp/mnt/USB_DRIVE/logs ro

# Start an encrypted file receiver in persistent mode (no timeout)
tailcatzero timeout 0
tailcatzero recv /tmp/mnt/USB_DRIVE/inbox
tailcatzero timeout 30  # Restore default
```

### 2. Managing Permission Escalation
```sh
# List all pending guest requests
tailcatzero requests

# Approve request #1 for the remainder of the session
tailcatzero approve 1

# Approve a request for single-use only
tailcatzero approve 1 --once

# Deny request #1
tailcatzero deny 1

# Proactively permit 'iperf3'
tailcatzero allow iperf3
```

### 3. Checking Status and Teardown
```sh
# Check running tunnels
tailcatzero status
```

*Expected output when sessions are active:*
```text
[🟢] Active TAILCAT ZER0 Sessions (2 Running):

  • 🔒 View-Only Diagnostic Shell      ⏱️ 26m remaining  PID: 14205
    Connect:  tailcat ssh tcpGFwWCBquiVYgLrL7k3HQDl...
    Requests: 🔔 1 Pending ('tailcatzero requests' or 'tailcatzero approve')

  • 📁 SFTP File Share (/jffs)         ⏱️ 26m remaining  PID: 14210
    Connect:  tailcat ls -l tcpGFwWCBwoiH7ENLsCVJqJ...
```

*Expected output when idle:*
```text
[⚪] No active TAILCAT ZER0 sessions running.
```

```sh
# Stop specific service
tailcatzero stop VIEW

# Stop everything immediately
tailcatzero stop all
```

---

## 🤖 Headless Scripting & Integration

### Example 1: Extract Capability Token in a Script
To extract the capability token programmatically (e.g. to send to a private Telegram bot or webhook):

```bash
#!/bin/sh
# Start a view-only diagnostic session
tailcatzero view > /tmp/tc_output.log 2>&1 &
sleep 2

# Extract token from active state file
TOKEN=$(grep "TOKEN=" /tmp/tailcat-view.env | cut -d'=' -f2)

if [ -n "$TOKEN" ]; then
    echo "Active Token: $TOKEN"
    # Example: Send to Telegram
    # curl -s -X POST "https://api.telegram.org/bot<TOKEN>/sendMessage" \
    #      -d chat_id="<CHAT_ID>" \
    #      -d text="Router View-Only Diagnostic Token: $TOKEN"
fi
```

### Example 2: Scheduled Daily Firmware / Backup Receiver
You can configure an Asuswrt `cru` cron job to automatically open a temporary 1-hour receiver inbox every Sunday at 3 AM:

```sh
cru a WeeklyReceiver "0 3 * * 0 /jffs/scripts/tailcatzero recv /tmp/mnt/USB_DRIVE/backups"
```
Because the default timeout automatically triggers teardown, the file receiver closes itself after the configured timeout period without manual intervention.
