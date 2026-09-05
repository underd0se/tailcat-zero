# 🛡️ View-Only Sandbox & Permission Escalation

The **Restricted View-Only Shell** in TAILCAT ZER0 is implemented by a dedicated hardened wrapper script: [`tailcat-view-shell`](file:///Users/Baris/tailcat-merlin/tailcat-view-shell).

It delivers a zero-trust diagnostic environment for external technicians, script developers, and forum helpers. It allows read-only visibility into system health, routing, NVRAM, and logs while strictly preventing state modification, data theft, and hardware corruption.

---

## 🏗️ Sandbox Architecture

When a guest connects to a view-only session, TailCat attaches their PTY to `tailcat-view-shell` instead of `/bin/sh`. Every command entered by the guest is intercepted, tokenized, and evaluated against multi-layer security rules before anything reaches the kernel.

```text
               +----------------------------------------+
               |        GUEST TERMINAL (TailCat)        |
               +----------------------------------------+
                                   |
                                   v
               +----------------------------------------+
               |           tailcat-view-shell           |
               +----------------------------------------+
                                   |
           +-----------------------+-----------------------+
           |                       |                       |
           v                       v                       v
    [Syntax Check]       [Path & Credential]        [GTFOBin Check]
 - No redirects (>, <)   - Block /etc/shadow        - No 'sort -o'
 - No subshells ($())    - Block .ssh/id_*          - No 'env <cmd>'
 - No chaining (; &&)    - Block dropbear keys      - No interactive
 - Pipe check (A | B)    - Filter 'nvram show'        pager breakouts
           |                       |                       |
           +-----------------------+-----------------------+
                                   |
                     Is command in Allowlist?
                                   |
                    +--------------+--------------+
                    |                             |
                 [ YES ]                       [ NO ]
                    |                             |
                    v                             v
           +------------------+         +-------------------+
           | Execute Pipeline |         | Blocked: Offer    |
           +------------------+         | 'request <cmd>'   |
                                        +-------------------+
```

---

## 📋 Default Allowed Commands

The sandbox provides access to all standard diagnostic tools:

| Category | Allowed Commands |
|---|---|
| **System Health** | `uptime`, `free`, `df`, `ps`, `top` (batch mode), `dmesg`, `sysinfo`, `uname`, `cat /proc/*` |
| **Network & WiFi** | `ip addr`, `ip route`, `ip neigh`, `ifconfig`, `netstat`, `route`, `ports`, `ping`, `mtr`, `wl`, `leases`, `wifi` |
| **NVRAM & Configs** | `nvram get <var>`, `nvram show` *(sanitized)* |
| **System Logs** | `logread`, `cat /tmp/syslog.log` |
| **Entware Package Queries** | `opkg list`, `opkg info`, `opkg status`, `opkg find`, `opkg search`, `opkg depends` |
| **Text Processing & Inspection** | `cat`, `head`, `tail`, `grep`, `egrep`, `fgrep`, `rg`, `sed`, `awk`, `cut`, `wc`, `diff`, `tree`, `sort`, `uniq` |
| **Pipelines** | Full Unix pipelines (`\|`) permitted between allowed inspection tools (e.g. `ps \| grep dnsmasq`) |

---

## 🚫 Blocked Syntax & Threat Mitigations

To prevent shell escapes and system tampering, the following syntax patterns are strictly rejected:

### 1. Output Redirection (`>`, `>>`, `<`)
* Guests cannot redirect command output to files or overwrite arbitrary locations.
* Example blocked: `echo "malicious" > /jffs/scripts/post-mount`

### 2. Command Chaining (`;`, `&&`, `||`)
* Commands must be single operations or standard Unix pipes (`|`). Chaining commands to sneak unallowed binaries past filters is blocked.
* Example blocked: `uptime; rm -rf /jffs`

### 3. Subshells & Command Substitution (`$()`, `` ` ``)
* Nesting command execution inside arguments or variables is forbidden.
* Example blocked: `cat $(which nvram)`

### 4. Sensitive File & Credential Protection
Access to router credential repositories is denied, even using `cat`, `head`, or `grep`:
* `/etc/shadow`, `/tmp/etc/shadow`, `/tmp/shadow`
* `/jffs/ssl/`, `/etc/dropbear/`, `/jffs/.ssh/id_*`
* `nvram show` and `nvram get` automatically filter and redact password and key variables (such as `http_passwd`, `wpa_psk`, `acc_webdav_password`).

### 5. GTFOBin Defenses
Many standard Unix utilities include secondary flags capable of writing files or invoking subshells. TAILCAT ZER0 explicitly hardens against these:
* **`sort -o <file>`**: Blocked because `-o` writes output directly to a file.
* **`uniq [input] [output]`**: Two-argument invocation blocked to prevent file overwrites.
* **`xxd -r`**: Reverse hex dump writing blocked.
* **`env <command>`**: Blocked from launching arbitrary child executables.
* **`less` / `more`**: Interactive pager shell breakout (`!`) is stripped or redirected to non-interactive streaming mode.

---

## 🛑 Hard Red Lines (Irreversible Hardware Safeguards)

Certain destructive commands can cause permanent flash memory corruption or brick router hardware. These commands are designated as **Hard Red Lines** and are **hard-blocked from ever being requested, approved, or executed under any circumstances**:

* `dd of=/dev/mtd*` or writing to any `/dev/mtdblock*`
* `flash_erase*` or `nandwrite`
* `rm -rf /` or `rm -rf /jffs`
* `nvram erase` or `nvram restore`

Even if the host admin attempts to run `tailcatzero allow "rm -rf /"`, the core sandbox engine will reject it with an irreversible hardware safeguard alert.

---

## 🔔 Live On-Demand Permission Escalation

If a remote assistant needs to run a diagnostic tool that is not in the default allowlist (e.g., `amtm`, `curl -I https://dns.google`, `traceroute 1.1.1.1`):

### 1. Guest Request Flow
When the guest attempts an unapproved command:

```text
view-shell:~$ traceroute 1.1.1.1
[-] Command 'traceroute' is restricted in view-only mode.
Would you like to request permission from the host router admin? [y/N]: y
[*] Permission request submitted (ID: #1). Waiting for host approval...
```
Or directly using the built-in `request` helper:
```text
view-shell:~$ request traceroute 1.1.1.1
[*] Permission request submitted (ID: #1). Waiting for host approval...
```

### 2. Host Alerting
The moment a request is submitted:
* A broadcast alert is immediately printed across all active host SSH terminals (`/dev/pts/*`):

```text
admin@RT-AX86U:/tmp/home/root# 
[🔔 TAILCAT ZER0 ALERT] View-only guest requested permission:
    Request ID:   #1
    Command:      traceroute 1.1.1.1
    Base Utility: traceroute
    Timestamp:    Sat Sep  5 22:15:30 2026
    Action:       Press [P] in dashboard or run: tailcatzero approve 1
```

* An event is logged to `syslog`:
```text
Sep  5 22:15:30 RT-AX86U tailcat-view-shell[30142]: Guest submitted permission request #1: 'traceroute 1.1.1.1'
```
* The TUI dashboard immediately updates with a prominent badge: `[P] 🔔 1 Pending Request`.

### 3. Host Approval Channels

#### Method A: Via Interactive TUI Modal
Press `P` on the main dashboard or active session card to open the **Pending Requests Modal**:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🔔 Pending Host Approval Request (1 of 1):

  Command:      traceroute 1.1.1.1
  Base Utility: traceroute
  Threat Level: LOW (Diagnostic inspection)
  Requested At: Sat Sep 5 21:55:00 2026

  Select Resolution:
  1. ⚡ Approve for Session    (Persists in allowlist until session teardown)
  2. ⏱️  Approve Once           (Permits one single execution only)
  3. ❌ Deny request           (Block execution & suppress repeat prompts)

========================================================================

  ⚡ Session (Default)  |  ⏱️ Once  |  ❌ Deny  |  ↩️ Back: 
```

If multiple requests are pending, TAILCAT ZER0 presents an interactive selection picker first:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🔍 2 Pending Guest Requests:

  1. traceroute 1.1.1.1 (traceroute) - Sat Sep 5 21:55:00 2026
  2. iperf3 -c 1.1.1.1 (iperf3) - Sat Sep 5 21:55:10 2026

========================================================================

  🔍 Select (1-2)  |  ↩️ Back: 
```

#### GTFOBin Threat Warnings:
If a guest requests a binary that contains known subshell or file-writing vectors (e.g. `find`, `awk`, `python`, `vi`, `tar`), the modal highlights a bold security warning:
```text
  ⚠️  SECURITY WARNING: 'find' is a known GTFOBin binary!
      Capable of subshell escapes (-exec, system). Review arguments carefully.
```

#### Method B: Via Headless CLI
Administrators managing the router via scripts or headless terminal sessions can inspect and resolve requests directly:

```text
admin@RT-AX86U:/tmp/home/root# tailcatzero requests
========================================================================
  🔔 Pending Guest Permission Requests
========================================================================
  ID  COMMAND              BASE UTILITY  THREAT  REQUESTED AT
  #1  traceroute 1.1.1.1   traceroute    LOW     Sat Sep  5 22:15:30 2026
========================================================================

admin@RT-AX86U:/tmp/home/root# tailcatzero approve 1
[✓] Approved request #1 ('traceroute 1.1.1.1') for this session.
[✓] View-shell guest notified and execution released.
```

CLI commands available:
```sh
# View all pending guest requests
tailcatzero requests

# Approve for the entire duration of the session
tailcatzero approve 1
# or by command name:
tailcatzero approve traceroute

# Approve for a single execution only
tailcatzero approve 1 --once

# Deny a request
tailcatzero deny 1

# Proactively permit or revoke a command
tailcatzero allow iperf3
tailcatzero revoke iperf3
```

### 4. Instant Execution
The moment the host approves the request, the guest's blocking prompt immediately unfreezes and executes:

```text
view-shell:~$ traceroute 1.1.1.1
[-] Command 'traceroute' is restricted in view-only mode.
Would you like to request permission from the host router admin? [y/N]: y
[*] Permission request submitted (ID: #1). Waiting for host approval...
[✓] Host approved 'traceroute 1.1.1.1' for this session!

traceroute to 1.1.1.1 (1.1.1.1), 30 hops max, 60 byte packets
 1  100.64.24.1 (100.64.24.1)  1.214 ms  1.108 ms  1.042 ms
 2  172.16.12.1 (172.16.12.1)  4.321 ms  4.110 ms  3.985 ms
 3  one.one.one.one (1.1.1.1)  8.112 ms  7.942 ms  7.810 ms

view-shell:~$ traceroute 8.8.8.8
traceroute to 8.8.8.8 (8.8.8.8), 30 hops max, 60 byte packets
 1  100.64.24.1 (100.64.24.1)  1.198 ms  1.084 ms  1.011 ms
 2  dns.google (8.8.8.8)       9.245 ms  8.812 ms  8.704 ms
```
