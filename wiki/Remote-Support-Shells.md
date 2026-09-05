# 🆘 Remote Support Shells

One of TAILCAT ZER0's most powerful capabilities is providing **zero-friction remote terminal access** to your router without opening WAN firewall ports, setting up DDNS, disclosing your router administrator password, or managing SSH public keys.

---

## 🆚 Root Shell vs. View-Only Shell

TAILCAT ZER0 offers two distinct shell operational modes depending on your trust level with the person connecting:

| Feature / Capability | 🟢 Full Root Shell (`ssh root`) | 🔒 Restricted View-Only Shell (`view`) |
|---|---|---|
| **Target User** | Router owner, co-admin, highly trusted technician | Forum helpers (e.g. SNBForums), community developers, untrusted diagnostic triage |
| **User Privileges** | Unrestricted `root` (`/bin/sh`) | Restricted operator (`tailcat-view-shell`) |
| **System Modification** | Full read/write access | ❌ Completely blocked (read-only sandbox) |
| **File Deletions / Writes** | Permitted (`rm`, `mv`, `cp`, `touch`, `dd`) | ❌ Blocked by sandbox wrapper & GTFOBin defenses |
| **Redirection / Subshells** | Permitted (`>`, `>>`, `$()`, `` ` ``) | ❌ Blocked by parser |
| **Sensitive File Access** | Unrestricted (`/etc/shadow`, `.ssh/id_*`) | ❌ Protected; access denied to credential stores |
| **Hardware Flash Safeguards** | Subject to normal root rules | 🛡️ Hard Red Lines block `mtd` flash tampering |
| **Permission Escalation** | Not applicable (already root) | 🔔 Supported (`request <cmd>` with host approval) |
| **Auto-Kill Watchdog** | Enabled (default 30m) | Enabled (default 30m) |

---

## 🔒 1. Restricted View-Only Diagnostic Shell (Recommended)

The **View-Only Shell** is built for zero-trust scenarios. When an external helper asks to check your router configuration or investigate an issue, you do not need to give them root privileges or your router password.

### Starting a View-Only Shell:

#### Via Interactive TUI:
1. Run `tailcatzero` and select **Option 1 (🆘 Remote Support Shell)**:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🆘 Remote Support Shell Options:

  1. 🔓 Full Root Shell (Read-Write)     Full admin shell with unrestricted root  [⚪ Inactive]
  2. 🔒 View-Only Diagnostic Shell       Safe inspection; writes blocked          [⚪ Inactive]

========================================================================

  🔓 Root (Default)  |  🔒 View-Only  |  ↩️ Back: 
```

2. Select **Option 2 (Restricted View-Only Shell)** (or press `v`).
3. The dedicated Active Session Card will display your capability token and copy-paste invite snippet:

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

#### Via Direct CLI Command:
```sh
tailcatzero view
# or
tailcatzero ssh view
```

### What the Guest Can Do:
* Inspect system resource utilization: `uptime`, `free`, `df`, `ps`, `top`, `dmesg`, `sysinfo`.
* Inspect network routing and interfaces: `ip addr`, `ip route`, `netstat`, `route`, `ports`, `ping`, `mtr`, `wl`, `leases`.
* Query NVRAM settings safely: `nvram get <var>`, `nvram show` (passwords & keys are automatically scrubbed).
* Read router logs: `logread`, `cat /tmp/syslog.log`.
* Query Entware packages: `opkg list`, `opkg info`, `opkg status`.
* Safe text filtering with pipelines: `ps | grep dnsmasq`, `nvram show | grep dhcp`.

### What the Guest Cannot Do:
* Cannot modify files or router settings (`nvram set`, `nvram commit`, `touch`, `rm`, `echo ... > file`).
* Cannot reboot, halt, or kill running processes (`reboot`, `kill`, `killall`).
* Cannot execute shell escapes, subshells, or command chaining (`;`, `&&`, `||`, `$()`).
* Cannot read passwords, private keys, or hashes (`/etc/shadow`, `/tmp/etc/shadow`, `id_rsa`, `dropbear.key`).

*(See [View-Only Sandbox & Escalation](View-Only-Sandbox-&-Permission-Escalation) for a deep dive into the sandbox architecture).*

---

## 🟢 2. Full Root Shell

The **Root Shell** provides complete, unrestricted administrative root access to the router's native BusyBox `/bin/sh` environment.

> [!WARNING]
> Only share Full Root Shell tokens with people you completely trust. A root shell has full authority to rewrite router flash partitions, modify firewalls, edit system configurations, and read stored credentials.

### Starting a Root Shell:

#### Via Interactive TUI:
1. Run `tailcatzero` ➔ select **Option 1 (🆘 Remote Support Shell)** ➔ select **Option 1 (Full Root Shell)** (or press `r` / `Enter`).
2. The Root Active Session Card is displayed:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🐱 TAILCAT ZER0 — Active Session
========================================================================

  Service:     🆘 Remote Support Shell (Root)
  Auto-Kill:   ⏱️ 30m remaining
  Security:    🔒 WireGuard P2P Encrypted (Zero WAN Ports Open)

  💬 Copy & Paste to Friend / Admin Support:
  ────────────────────────────────────────────────────────────────────────
  Hey, I've opened a temporary TailCat session on my router (30m remaining).
  Run: tailcat ssh tcpGFwWCCuqKFX5cJxm-pNdgo2ILkJgpEGQO3cofQPT4fhKulsPGFrWCCJzce8x0-_mKErEp7pzaVaP_8ugclQvlwiwn0nCDBAZWFxWCBAelM3VvuSlnH6mIe_NpCTqtGxv1-S0oVPZ03h3-t4emFpGQEv
  ────────────────────────────────────────────────────────────────────────

========================================================================

  🛑 Stop  |  📱 QR Code  |  🔄 Refresh  |  ↩️ Back: 
```

#### Via Direct CLI Command:
```sh
tailcatzero ssh root
# or simply:
tailcatzero ssh
```

---

## 💻 How the Guest Connects

The connecting user does **not** need a Tailscale account, VPN profile, or SSH key. They only need the `tailcat` client installed on their machine:

### 1. Install Client
* **macOS:**
  ```sh
  brew install tailcat
  ```
* **Linux / Go:**
  ```sh
  go install github.com/tailscale/tailcat/cmd/tailcat@latest
  ```
* **Precompiled Binaries:** Download for Linux, macOS, or Windows from [Tailscale TailCat Releases](https://github.com/tailscale/tailcat/releases).

### 2. Connect via Token
The guest simply runs:
```sh
tailcat ssh tcXXXXXXXXXXXX
```

The client negotiates an encrypted WireGuard peer connection via Tailscale's DERP relay network, punches through any NATs or firewalls, and attaches to the remote shell within seconds.

---

## 🛑 Terminating a Session

As the host router administrator, you maintain absolute control over every active session:

* **From TUI:** Press `v` (View Sessions) ➔ Press `s` (Stop Session).
* **From CLI:**
  ```sh
  # Stop view-only shell
  tailcatzero stop VIEW

  # Stop root shell
  tailcatzero stop SSH

  # Stop all active sessions
  tailcatzero stop all
  ```

Upon termination, the background WireGuard process is killed immediately, all temporary session sockets are deleted, and the capability token is revoked permanently. Any attempt to reconnect with the old token will fail.
