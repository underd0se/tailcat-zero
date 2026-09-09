# 🔒 Security Architecture & Threat Model

**TAILCAT ZER0** is designed from the ground up around zero-trust and ephemeral security principles. This document provides a deep technical analysis of its cryptographic foundations, network traversal model, process containment, and threat model.

---

## 🔐 1. Cryptographic Foundations

TAILCAT ZER0 utilizes [Tailscale's TailCat](https://github.com/tailscale/tailcat) engine, which implements the **WireGuard** protocol using the **Noise Protocol Framework**:

* **Key Exchange:** Curve25519 (ECDH)
* **Symmetric Encryption:** ChaCha20-Poly1305 authenticated encryption with associated data (AEAD)
* **Hashing & Authentication:** BLAKE2s
* **Capability Tokens:** Cryptographically random 256-bit capability strings generated via the OS entropy pool (`/dev/urandom`).

### Capability-Based Security vs Identity-Based Security
In traditional SSH setups, access is identity-based (requiring user accounts, passwords, and authorized keys). 

In TAILCAT ZER0, access is **capability-based**:
* Possession of the capability token confers immediate, fine-grained permission to connect to that specific service instance.
* Tokens are single-purpose, ephemeral, and instantly revocable by the host at any moment.
* Zero credentials (passwords, private SSH keys, admin logins) are ever disclosed to the guest.

---

## 🌐 2. Network Topology & NAT Traversal

```text
[Client Device]                                         [Asuswrt Router]
       |                                                       |
       | 1. Outgoing TLS (Port 443)                            | 1. Outgoing TLS (Port 443)
       +-----------------------+       +-----------------------+
                               |       |
                               v       v
                     +---------------------------+
                     | Tailscale DERP Relays     |
                     | (Global Anycast Fabric)   |
                     +---------------------------+
                                   |
                     [Rendezvous & Hole Punching]
                                   |
         +-------------------------+-------------------------+
         |                                                   |
         v                                                   v
[Direct UDP P2P WireGuard]                         [Encrypted DERP Fallback]
(If NAT permits direct path)                      (If symmetric NAT/CGNAT blocks UDP)
```

### Key Network Properties:
1. **Zero Open WAN Ports:** TAILCAT ZER0 establishes outbound-only connections to Tailscale DERP relays over standard HTTPS (TCP 443) or STUN (UDP 3478). Your router's WAN firewall rules (`iptables` / `ip6tables`) remain completely unaltered.
2. **End-to-End Encryption (E2EE):** When traffic traverses Tailscale's global DERP relay servers, DERP nodes only see encrypted WireGuard packets. They cannot decrypt, inspect, or tamper with your terminal sessions, file transfers, or WebGUI traffic.
3. **Seamless NAT Hole-Punching:** Uses `magicsock` to automatically transition from relay to direct peer-to-peer UDP WireGuard connections whenever possible.

---

## 🧠 3. Volatile State & Permission Hardening

Router security tools must not leave accidental backdoors open if power is lost or a process crashes. TAILCAT ZER0 strictly enforces volatile memory storage and restrictive POSIX permissions:

* **Session Locks & PIDs:** Stored by default in volatile memory (`/tmp/tailcat_sessions/`), which is backed by router RAM (`tmpfs`).
* **Strict Filesystem Permissions:**
  * Runtime directories (`/tmp/tailcat_sessions/`, `requests/`, `VIEW_ONCE/`, `active_tuis/`) are strictly locked down to `0700` (`rwx------`), preventing local unprivileged users or compromised daemons from snooping IPC state.
  * Capability address files and `.env` session profiles are restricted to `0600` (`rw-------`).
  * Configuration files (`tailcatzero.cfg`) in `/jffs/addons/tailcatzero/` are set to `0600`.
* **Clean Boot Guarantee (Default):** Standard sessions leave zero remnants across reboots. If power is interrupted, all tunnels terminate and volatile memory state is automatically wiped.
* **Reboot-Persistent Sessions (`timeout: 0` / Opt-In):**
  * When sessions are explicitly started in persistent mode (`timeout: 0`), service parameters and tokens are written to `/jffs/addons/tailcatzero/persistent/${service_type}.conf` with directory permissions `0700` and file permissions `0600`.
  * Restored automatically via Asuswrt-Merlin's `/jffs/scripts/wan-event` hook (`[ "$2" = "connected" ]`), guaranteeing that the WAN interface and NTP time synchronization (`date +%s > 1700000000`) are active before launching tunnels.
  * **amtm Mail Integration Security:** Integrates directly with Asuswrt-Merlin's amtm mail framework (`/jffs/addons/amtm/mail`). Encrypted SMTP passwords (`emailpw.enc`) are decrypted in-flight via `/usr/sbin/openssl aes-256-cbc`; plaintext credentials are never copied or stored in TAILCAT ZER0 configs.
  * **Single Combined Notification:** When persistent sessions restart and generate new WireGuard keys, a single combined email is sent with all refreshed tokens, eliminating email notification storms.
  * **View-Only Session Safety Boundary:** Diagnostic view-only sessions (`VIEW`) are strictly excluded from persistent storage and are unconditionally capped at 120 minutes maximum.

---

## 🔒 4. Host IPC Isolation & Parser Defense

When view-only guests request elevated permissions via `request <command>`, IPC communication flows via request files (`.req`) and response tokens (`.resp`) in `/tmp/tailcat_sessions/requests/`.

* **Deterministic Key-Value Parsing:** The router host and view shell parse `.req` and `.env` files using deterministic, line-by-line key-value parsers (`IFS='=' read -r key val`).
* **Zero Shell Sourcing:** Sourcing external files (`. "$file"`) and `eval` on serialized state are strictly prohibited, preventing guests from embedding command substitutions (`$(...)`) into serialized request attributes.

---

## ⏱️ 5. The Ephemeral Watchdog

Every session launched by TAILCAT ZER0 is assigned a dedicated background watchdog process.

1. At startup, the watchdog records the creation timestamp, calculated expiration time, and kernel PID of the service process.
2. **Phased Expiration Warnings:** If the timeout exceeds 5 minutes, the watchdog proactively broadcasts warnings prior to session termination:
   * **5 Minutes Remaining:** Broadcasts a notice to active host TUIs (`/tmp/tailcat_sessions/active_tuis`) and remote guest PTYs (`/dev/pts/*`).
   * **1 Minute Remaining:** Broadcasts a high-priority alert (`⚠️ Warning: Session will terminate in 60 seconds`).
3. **PID Rollover & Identity Validation:** Before issuing any kill signals, the watchdog executes `is_tailcat_process("$pid")`, inspecting `/proc/$pid/comm` and `/proc/$pid/cmdline`. If the original process died and the Linux kernel recycled the PID to another system daemon (e.g. `dnsmasq`, `httpd`), `kill -9` is strictly suppressed to prevent friendly fire.
4. **Graceful SIGTERM Teardown:** `stop_single_session()` sends `SIGTERM` first, giving the watchdog subshell time to fire its `trap 'kill $SLEEP_PID; exit 0' TERM INT HUP` handler and reap any tracked `sleep` child processes. `SIGKILL` is issued only after a 0.1 s grace period if the process has not yet exited, eliminating zombie `sleep` processes after session teardown.
5. **Clean Teardown:** Session files, address files, and approval tokens are unlinked, rendering the capability token instantly dead.

---

## 🛡️ 6. Threat Model Analysis

| Threat Scenario | Potential Impact | TAILCAT ZER0 Defense |
|---|---|---|
| **Token Interception / Leaked Token** | Unauthorized connection to running session | *Tokens are ephemeral (auto-kill in 30m) and can be revoked instantly via `tailcatzero stop` or pressing `s` in TUI.* |
| **Malicious Guest in Root Shell** | Full router compromise | *Zero-Trust Inversion & Confirmation Gate: Option 1 defaults to View-Only Diagnostic Shell. Full Root Shell requires explicit deliberate opt-in and typing 'YES' to an interactive security warning modal.* |
| **PID Rollover / "Friendly Fire" Kill Trap** | Terminating unrelated system daemons upon timeout | *`is_tailcat_process()` validates process identity via `/proc/<pid>/comm` and `/proc/<pid>/cmdline` before executing kill signals.* |
| **Flash Storage (JFFS) Wear Churn** | Premature NAND/SPI flash exhaustion from repeated runs | *`ensure_file_perm_600()` uses `stat` to check permissions before invoking `chmod 600`, eliminating redundant inode metadata writes.* |
| **Sandbox Breakout via Filter Obfuscation** | Executing unauthorized commands | *Pre-parse canonicalization strips quotes and backslashes before validation, preventing quote-splitting and backslash-escape filter evasions.* |
| **Wildcard Glob File Extraction** | Dumping `/etc/shadow` or `/etc/passwd` | *Multi-layer path checks inspect stage strings, token paths, and filesystem glob expansions (`cat /etc/pas*`), blocking glob-based credential access.* |
| **Direct Hardware Memory / Partition Access** | Extracting keys or firmware corruption | *Access to raw device nodes (`/dev/mtd*`, `/dev/mem`, `/dev/kmem`, `/dev/port`, `/proc/kcore`) is blocked across all tools.* |
| **Host Privilege Escalation via IPC** | Root command execution by host admin | *Host IPC uses deterministic line-by-line parsing; shell sourcing (`. "$file"`) of guest `.req` files is eliminated.* |
| **Multi-Variable NVRAM Extraction** | Stealing admin passwords via trailing arguments | *All arguments in `nvram get` are evaluated; account lists (`acc_list`) and certificate keys are restricted.* |
| **GTFOBin Flag Exploits** | File writing or subshell spawning via allowed tools | *Flag-level checks prohibit `-o`/`--output` (`sort`, `tree`), `--diff-program` (`diff`), `-c` (`dmesg`), `-f` (`ping`), and `-s` (`date`).* |
| **Background Execution / Chaining** | Running commands outside parser containment | *Backgrounding (`&`) and command chaining (`;`, `&&`, `||`) are prohibited; only linear pipelines (`\|`) with allowed tools are permitted.* |
| **Hardware Flash Corruption** | Permanent router bricking | *Hard Red Lines strictly prohibit `dd of=/dev/mtd*`, `flash_erase*`, `rm -rf /`, `nvram erase`, and raw memory nodes from being requested or approved.* |
| **Sensitive File Access via Request Escalation** | Stealing `/etc/shadow` or keys via `request` | *`request_command_approval()` inspects canonicalized inputs with `is_sensitive_file_access()`, rejecting credential requests before IPC creation.* |
| **Symlink Traversal to Credentials** | Accessing `/etc/shadow` via symlinks | *All path tokens are inspected and resolved to canonical paths via `readlink -f`, blocking direct and multi-hop symlinks.* |
| **Character Device Flooding / DoS** | Saturating tunnel bandwidth and pinning CPU | *Direct reads from raw streaming devices (`/dev/zero`, `/dev/urandom`, `/dev/console`) and kernel diagnostic feeds (`/proc/kmsg`, `/proc/kallsyms`) are prohibited; `/dev/null` is safely preserved.* |
| **Pipeline Process Killing via `top`** | Terminating router processes from view-only shell | *Sanitized pipeline builder forces non-interactive batch mode (`-b -n 1` on Linux, `-l 1 -n 0` on macOS) across all pipeline stages.* |
| **Pager Shell Escape via Direct Paths** | Spawning root shells via `/bin/less` or `/usr/bin/more` | *Pipeline stages are rewritten to `safe_stream()`, stripping non-portable flags and streaming exclusively through `cat`.* |
| **MITM on Relay Network** | Eavesdropping on session traffic | *Traffic is protected with Noise/WireGuard end-to-end encryption. DERP relays have zero visibility into plaintext data.* |
| **Local Privilege Snooping in `/tmp`** | Unauthorized process reading active tokens | *Directory permissions enforced at `0700` and address files at `0600`.* |
| **Dynamic Linker Hijacking (`LD_PRELOAD`)** | Hijacking child process execution via preloaded shared libraries | *`main()` strips `LD_PRELOAD`, `LD_LIBRARY_PATH`, `LD_AUDIT`, `DYLD_*`, and `BASH_ENV` before executing child binaries.* |
| **Network Route / Address Flush DoS** | Wiping routing tables and disconnecting interfaces | *`ip route flush` and `ip addr flush` are blacklisted, preserving tunnel connectivity and interface routing.* |
| **Broadcom Radio Shutdown DoS** | Disabling router Wi-Fi radios from view shell | *`wl radio off`, `wl channel`, `wl ssid`, `wl reinit`, and `wl reset` are prohibited in view-only mode.* |
| **Option-Embedded Symlink Traversal** | Reading sensitive files via `--file=/path` or `-f/path` | *Tokens with embedded paths (`=`, `-f`) are resolved through canonical `realpath()` checks; `sort --files0-from` is hard-blocked.* |
| **Process Memory / Credential Extraction** | Stealing NVRAM secrets from `/proc/<pid>/environ` | *Access to `/proc/<pid>/environ`, `/proc/<pid>/mem`, `/proc/<pid>/cmdline`, and `/proc/<pid>/fd/*` is completely blocked.* |
| **`/proc/meminfo` False Positive Blocking** | Denying access to safe memory stats | *`check_proc_component()` matches only full path components, allowing `/proc/meminfo` while still blocking `/proc/<pid>/mem`.* |
| **Sensitive File Dump via `cut` / `column`** | Extracting `/etc/shadow` fields with text tools | *`cut` and `column` are routed through `is_sensitive_file_access()` checks, blocking access to credentials and restricted paths.* |
| **Directory Traversal via `cd`** | Navigating into sensitive directories from view shell | *`cd` is a native in-process built-in (`chdir()`) guarded by `check_file_path_security()`, blocking paths under `/jffs/ssl`, `/etc/ssl`, and similar restricted trees.* |
| **Persistent Configuration Tampering** | Viewing or modifying saved reboot tunnels from view shell | *`persistent` and `persistent_sessions` are hardcoded into sensitive path patterns; `tailcat` and `tailcatzero` execution requests are unconditionally rejected.* |
| **amtm Email Credential Snooping** | Extracting encrypted SMTP passwords or configs | *`email.conf`, `emailpw.enc`, and `/jffs/addons/amtm/mail` paths are blocked by sandbox sensitive pattern filters.* |
| **View-Only Session Infinite Persistence** | Guest keeping permanent backdoor across reboots | *`VIEW` sessions are strictly capped at 120m max (even if timeout: 0) and `save_persistent_session()` rejects saving VIEW to flash.* |


