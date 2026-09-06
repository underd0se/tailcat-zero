# 🛡️ View-Only Sandbox & Permission Escalation

The **Restricted View-Only Shell** in TAILCAT ZER0 is implemented by a dedicated hardened C99 binary statically linked with Musl libc: [`src/tailcat-view-shell.c`](file:///Users/Baris/tailcat-merlin/src/tailcat-view-shell.c) (distributed via [`bin/tailcat-view-shell-<arch>`](file:///Users/Baris/tailcat-merlin/bin)).

It delivers a zero-trust diagnostic environment for external technicians, script developers, and forum helpers. It allows read-only visibility into system health, routing, NVRAM, and logs while strictly preventing state modification, data theft, and hardware corruption. Because it is compiled in C99 without dynamic allocations, it completely bypasses `/bin/sh` and `eval`, executing pipelines directly via POSIX kernel syscalls.

---

## 🏗️ Sandbox Architecture

When a guest connects to a view-only session, TailCat attaches their PTY directly to `tailcat-view-shell` instead of `/bin/sh`. Every command entered by the guest is tokenized directly into an argument vector (`argv[]`) and evaluated against multi-layer security rules before anything reaches the kernel:

```text
               +----------------------------------------+
               |        GUEST TERMINAL (TailCat)        |
               +----------------------------------------+
                                   |
                                   v
               +----------------------------------------+
               |           tailcat-view-shell           |
               |       (C99 Musl Static Binary)         |
               +----------------------------------------+
                                   |
           +-----------------------+-----------------------+
           |                       |                       |
           v                       v                       v
    [Syntax Check]       [Path & Device Check]      [GTFOBin Flag Check]
 - No redirects (>, <)   - Block /etc/shadow, passwd - No 'sort -o', 'tree -o'
 - No subshells ($())      (including wildcard globs)- No 'diff --diff-program'
 - No chaining (; && ||) - Block raw /dev/mtd*, mem  - No 'dmesg -c', 'ping -f'
 - No backgrounding (&)  - Block .ssh, dropbear keys - No 'date -s'
 - Validated pipelines   - Filter multi-var nvram    - Streamless pagers
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
| **NVRAM & Configs** | `nvram get <var>` *(credential and account list keys blocked)*, `nvram show` *(sanitized)* |
| **System Logs** | `logread`, `cat /tmp/syslog.log` |
| **Entware Package Queries** | `opkg list`, `opkg info`, `opkg status`, `opkg find`, `opkg search`, `opkg depends` |
| **Text Processing & Inspection** | `cat`, `head`, `tail`, `more`, `less`, `grep`, `egrep`, `fgrep`, `rg`, `cut`, `column`, `tr`, `stat`, `file`, `strings`, `hexdump`, `wc`, `diff`, `tree`, `sort`, `uniq` *(Note: `awk` and `sed` are classified as GTFOBins and require host approval via `request`)* |
| **Pipelines** | Full Unix pipelines (`\|`) permitted between allowed inspection tools (e.g. `ps \| grep dnsmasq`) |

---

## 🚫 Blocked Syntax & Threat Mitigations

To prevent shell escapes and system tampering, the following security controls are strictly enforced:

### 1. Output Redirection (`>`, `>>`, `<`)
* Guests cannot redirect command output to files or overwrite arbitrary locations.
* Example blocked: `echo "malicious" > /jffs/scripts/post-mount`

### 2. Command Chaining & Background Execution (`;`, `&&`, `||`, `&`)
* Commands must be single operations or standard Unix pipes (`|`). Chaining commands or backgrounding tasks with `&` to bypass parser containment is prohibited.
* Example blocked: `uptime; rm -rf /jffs` or `uptime & touch /tmp/pwned`

### 3. Subshells & Command Substitution (`$()`, `` ` ``)
* Nesting command execution inside arguments or variables is forbidden.
* Example blocked: `cat $(which nvram)`

### 4. Input Canonicalization (Quote & Backslash Defense)
* Attackers often split strings across quotes or backslashes (e.g. `cat '/tmp/etc/sha''dow'`, `route "add"`, `wl \down`) to evade regex filters.
* All input tokens are canonicalized (quotes and backslashes stripped, whitespace normalized) prior to validation.

### 5. Sensitive File, Symlink, Memory & Escalation Protection
Access to router credential repositories, process memory, and private keys is strictly prohibited:
* `/etc/shadow`, `/tmp/etc/shadow`, `/etc/passwd`, `/etc/master.passwd`
* Wildcard attempts like `cat /etc/pas*` or `grep root /tmp/etc/sha*` are expanded and validated against the sensitive blacklist before execution.
* **Symlink Resolution & Option Embedding:** All tokens and option-embedded file paths (e.g. `--file=/path`, `-f/path`) are inspected and resolved to their canonical target paths using `readlink` and `realpath`, preventing both direct, multi-hop, and flag-embedded symlink evasions.
* **Process Memory & Credential Extraction Defense:** Access to `/proc/<pid>/environ` (exposing process environment variables and plaintext NVRAM secrets), `/proc/<pid>/mem`, `/proc/<pid>/cmdline`, and `/proc/<pid>/fd/*` is strictly blocked.
* **Request Escalation Defense:** Requests targeting sensitive system files or credentials (e.g. `request cat /etc/shadow` or pipeline-smuggled `request echo ok | cat /etc/shadow`) are immediately rejected before request creation.
* **Strict Sensitive Isolation:** Approving a tool session-wide (e.g. `cat` or `grep`) never whitelists sensitive files; inspection utilities remain barred from reading credential repositories under all conditions.
* `/jffs/ssl/`, `/etc/dropbear/`, `/jffs/.ssh/id_*`, `/jffs/.sys*`, WireGuard configs (`/etc/wireguard/`, `*.ovpn`).
* Session runtime tokens (`tailcat_sessions`, `tailcat_addr_*.txt`, `tailcatzero.cfg`).

### 6. Hardware Flash, Device Node & Kernel Protection
Direct access to raw block/character device nodes and sensitive kernel diagnostic endpoints is blocked:
* `/dev/mtd*`, `/dev/mtdblock*`, `/dev/ubi*` (raw flash partitions containing firmware, Wi-Fi keys, and root hashes).
* `/dev/mem`, `/dev/kmem`, `/dev/port`, `/proc/kcore` (physical and kernel RAM).
* `/dev/sd*`, `/dev/nvme*`, `/dev/mmcblk*` (raw storage disk devices).
* **Character Device Flood & DoS Defense:** Direct reads from raw character devices (`/dev/zero`, `/dev/urandom`, `/dev/random`, `/dev/console`, `/dev/tty*`) are prohibited to prevent tunnel saturation and CPU starvation, while explicitly preserving `/dev/null`.
* `/proc/kmsg` (prevents blocking and ring buffer consumption) and `/proc/kallsyms` (prevents kernel symbol address disclosure).

### 7. Network & Interface Mutation Prevention
* **Route & Address Flush DoS:** `ip route flush` and `ip addr flush` are explicitly blocked, preventing malicious or accidental routing table wiping and interface disconnects.
* **Leading Option Support:** Subcommand argument parsing skips leading flags (e.g. `ip -4 route`, `ip -6 addr`, `ip -br link`), ensuring diagnostic inspections work seamlessly while checking all tokens for mutation verbs (`add`, `del`, `delete`, `change`, `replace`, `set`, `flush`, `save`, `restore`).
* **Broadcom Wi-Fi Radio Shutdown:** `wl radio off`, `wl channel`, `wl ssid`, `wl reinit`, and `wl reset` are blocked, preventing guests from shutting down wireless radios or disrupting client connections.
* **Interface Configuration Mutations:** `ifconfig` blocks interface state toggling (`up`/`down`), IPv6 address addition/deletion (`add`/`del`), and promiscuous mode manipulation.

### 8. NVRAM Multi-Variable Query & Credential Filtering
* `nvram get` validates **all** trailing arguments in multi-key queries (`nvram get lan_ipaddr http_passwd`), preventing password extraction via trailing parameters.
* **Extended Credential Filtering:** Filters cover `pass` (including `http_pass`, `admin_pass`), `cert` (`vpn_client1_cert`, `https_cert`), `priv` (`wgc1_priv`), `ovpn` (`vpn_client1_ovpn`), `wg` (`wgs_priv`), `hash`, `salt`, `secret`, `key`, `token`, `auth`, `user`, `cookie`, `session`, `otp`, `totp`, `api`, and account lists (`acc_list`, `acc_webdavusers`) unconditionally.

### 9. GTFOBin & In-Tool Flag Defenses
Standard Unix utilities with secondary execution or write capabilities are neutralized:
* **Dynamic Linker Sanitization:** `main()` strips `LD_PRELOAD`, `LD_LIBRARY_PATH`, `LD_AUDIT`, and `DYLD_*` before executing any child process.
* **`sort -o <file>` / `--compress*` / `--files0-from`**: Blocked to prevent file writing, external compressor execution, and option-embedded file reading, including GNU prefix evasions (`--compress-prog`).
* **`tree -o <file>`**: Output file redirection blocked.
* **`diff --diff*`**: External comparison binary execution blocked, including GNU prefix evasions (`--diff-prog`).
* **`uniq [input] [output]`**: Positional output destination arguments blocked.
* **`dmesg -c` / `-C`**: Kernel ring buffer clearing blocked.
* **`ping -f`**: ICMP flood denial of service blocked.
* **`date -s` / `--set`**: System clock mutation blocked.
* **`env <command>`**: Blocked from launching arbitrary child executables.
* **`top`**: Automatically rewritten with non-interactive batch flags (`-b -n 1` on Linux, `-l 1 -n 0` on macOS) across all pipeline stages, preventing interactive process signal killing (`k`).
* **`less` / `more`**: Pipeline commands invoking `less`, `more`, or explicit filesystem paths (`/bin/less`, `/usr/bin/more`) are rewritten through `safe_stream()`, filtering non-portable flags and streaming via `cat` to eliminate pager shell breakouts (`!`).

---

## 🛑 Hard Red Lines (Irreversible Hardware Safeguards)

Certain destructive commands can cause permanent flash memory corruption or brick router hardware. These commands are designated as **Hard Red Lines** and are **hard-blocked from ever being requested, approved, or executed under any circumstances**:

* `dd of=/dev/mtd*` or writing to any `/dev/mtdblock*`
* Accessing `/dev/mem` or `/dev/kmem`
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
  TAILCAT ZER0 v1.10.0             ╱|、
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
  TAILCAT ZER0 v1.10.0             ╱|、
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
