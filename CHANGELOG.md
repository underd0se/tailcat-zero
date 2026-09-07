# Changelog ─ TAILCAT ZER0

All notable changes to TAILCAT ZER0 are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.11.0] - 2026-09-07

### Interactive Linenoise Line Editing, Tab Autocompletion & Symlink Security Hardening

* **Embedded Zero-Dependency Line Editing & History Engine (`src/linenoise.c`, `src/linenoise.h`):**
  * Embedded Salvatore Sanfilippo's `linenoise` library directly into the C99 Musl static build pipeline, eliminating external runtime library requirements and maintaining tiny binary footprints (~160 KB).
  * In-memory command history with Up/Down arrow key recall (100 commands max).
  * Seamless non-interactive fallback mode for headless pipelines and automated scripts.
* **Context-Aware Tab-Completion (`src/tailcat-view-shell.c`):**
  * **Command Completion**: Autocompletes base commands from the allowlist (`allowed_commands`) at prompt start or following pipeline stages (`|`).
  * **Path & File Completion**: Autocompletes filesystem entries, adding trailing slashes (`/`) for directories to enable rapid continued navigation and trailing spaces for regular files.
  * **Context-Aware `cd`**: Limits completions strictly to directory targets when completing arguments to `cd`.
  * **Tilde (`~`) Path Expansion**: Supports `~` and `~/...` directory autocompletion.
* **🛡️ Recursive Multi-Hop Symlink Security Resolution:**
  * Implemented `check_symlink_chain_security()` to recursively resolve up to 16 hops of relative and absolute symlinks, verifying every intermediate link and the final target against sensitive credential patterns.
  * Ensures that symlink evasion attempts (e.g. `hop2 -> hop1 -> /etc/shadow`) never leak or suggest sensitive files, even if intermediate names do not contain sensitive keywords or if targets are broken.
* **Directory Enumeration & Metacharacter Sanitization:**
  * Added validation in `shell_completion()` to discard filenames containing control characters (`< 32`, `127`, `\r`, `\n`) or shell metacharacters (`;`, `$`, `` ` ``) before display or buffer insertion, preventing ANSI injection or command smuggling.
* **Comprehensive Automated Security Audit Suite (`tests/unit/test_security_audit.py`):**
  * Implemented 34-point PTY-based automated security audit covering command allowlist isolation, sensitive file masking, symlink traversal evasion, and context-aware directory completions.

## [1.10.4] - 2026-09-07

### Dynamic Path Prompt & Navigation UX

* **Dynamic Working Directory Prompt (`src/tailcat-view-shell.c`):**
  * Integrated dynamic `getcwd()` resolution into the main shell loop via `get_prompt_dir()`. The prompt now continuously reflects directory changes instead of displaying a static `~$`.
  * Path collapsing replaces `$HOME` with `~` (or `~/subdir`), matching standard Bash/Zsh UX with high-contrast color formatting (`tailcatzero-view:/jffs/scripts$ `).
* **Tilde (`~`) Expansion in `cd` Built-in:**
  * Implemented in-process tilde expansion for the `cd` built-in, allowing operators to run `cd ~` or `cd ~/path` directly without requiring shell expansion.

## [1.10.3] - 2026-09-06

### Obscure Networking DoS Remediation

* **Advanced `ip` Route Mutation Blocking:**
  * Blocked `append`, `prepend`, and `insert` verbs in the `ip` command to prevent obscure route manipulation attacks that evaded the standard `add`/`del` filters.
* **Advanced `ifconfig` Interface Mutation Blocking:**
  * Blocked `txqueuelen`, `name`, `multicast` (`-multicast`), and `allmulti` (`-allmulti`) flags in `ifconfig` to prevent advanced interface state manipulation and queue flooding attacks.

## [1.10.2] - 2026-09-06

### Deep Security Review & Remediation

* **Removed `eval` from Request Handling:**
  * Replaced `eval` array manipulation logic in `show_approval_modal()` with strict POSIX `for`-loops. This eliminates the risk of double-expansion command injection (e.g. `$(reboot)`) if an attacker were to manipulate the names of request files in the `/tmp` directory.
* **Eliminated TOCTOU IPC File Races:**
  * Replaced statically named `.tmp` files during approval and denial list mutations with process-ID suffixed files (`.$$.tmp`). This ensures isolated state writes during concurrent admin UI operations, preventing mutual overwrites.
* **GNU `date -f` File Leakage Protection:**
  * Added the `date` command to the `is_sensitive_file_access` validation logic within `tailcat-view-shell.c`. GNU `date` via Entware includes a `--file` (`-f`) flag which could have been abused to leak file contents (like `/etc/shadow`) via "invalid date" stderr parsing. Such queries are now strictly blocked.



## [1.10.1] - 2026-09-06

### Bug Fixes & Security Hardening

* **`is_tailcat_process()` False Positive on Development Hosts:**
  * Basename-extracted `/proc/<pid>/cmdline[0]` and `/proc/<pid>/cmdline[1]` via `${_pcmd##*/}` before matching `*tailcat*`. Previously, a script running from a directory whose **path** contained `tailcat` (e.g. `~/tailcat-merlin/tests/`) was misidentified as a tailcat process, causing watchdog kills to abort the current shell.
  * BSD/macOS `ps -o comm=` / `ps -o command=` fallback paths apply the same basename normalization.
* **`get_webgui_connect_info()` Port Parsing Glitch:**
  * Replaced the loose `grep -oE '[0-9]+'` fallback with protocol-aware stripping (`${w_info#*://}`), then matching only a colon-prefixed port or a purely numeric token. A naked IP URL like `http://192.168.1.1` previously extracted `192` (first octet) as the port; it now correctly defaults to `80` (HTTP) or `8443` (HTTPS).
* **`save_timeout_config()` Destructive Overwrite:**
  * Replaced full config file overwrite with `sed -i` in-place key update, preserving any user-defined `DERP_URL` or custom keys. Falls back to `>>` append if the key is absent, or creates a fresh file only when none exists.
* **Watchdog Orphaned `sleep` Processes:**
  * Watchdog subshells now set `trap 'kill $SLEEP_PID 2>/dev/null; exit 0' TERM INT HUP` and use a `_safe_sleep()` wrapper that tracks the child `sleep` PID and reaps it on SIGTERM, eliminating zombie `sleep` processes after `tailcatzero stop`.
  * `stop_single_session()` sends `SIGTERM` first (allowing the trap to fire), then `SIGKILL` after a 0.1 s grace period if the process is still alive.
  * `FILES` branch tunnel args are now re-quoted cleanly before passing to the watchdog subshell, fixing word-splitting on USB mount paths containing spaces.
* **`/proc/meminfo` False Positive in View Shell:**
  * Replaced loose substring `case_str_search(p, "mem")` with a new `check_proc_component()` path-component matcher that requires a preceding `/` and a trailing `/`, `\0`, or whitespace. `/proc/meminfo` is now correctly permitted; `/proc/<pid>/mem` remains blocked.
* **`cut` and `column` Sensitive File Disclosure:**
  * Added `cut` and `column` to the `is_sensitive_file_access()` branch in `validate_stage()`. Both commands now go through the same credential and sensitive-path checks as `cat`, `head`, `grep`, etc., closing a bypass where `cut -d: -f1 /etc/shadow` was permitted.
* **`pwd` and `cd` Built-ins in View Shell:**
  * Implemented `pwd` (calls `getcwd()`) and `cd` (calls `chdir()` with `check_file_path_security()` guard) as native in-process built-ins, eliminating the need for shell invocation. `cd` to sensitive paths (e.g. `/jffs/ssl`, `/etc/ssl`) is blocked. Both commands are added to the `allowed[]` and `allowed_req[]` arrays.


## [1.10.0] - 2026-09-06


### Zero-Trust UX Defaults, Flash Wear Elimination, PID Rollover Hardening & Expiration Warnings

* **Zero-Trust Default Inversion & Explicit Root Confirmation (`start_ssh_menu`):**
  * **View-Only as Default:** Inverted the remote support shell options in the TUI dashboard so Option 1 (Default) launches the safe **🔒 View-Only Diagnostic Shell** with all mutation commands strictly sandboxed.
  * **Deliberate Opt-in for Root Access:** Full Root Shell (Option 2) now requires explicit deliberate opt-in: displays an interactive security warning modal highlighting risk of credential exposure and flash reconfiguration, requiring typing `YES` to proceed.
* **PID Rollover & "Friendly Fire" Termination Safeguards:**
  * **Process Identity Validation:** Added `is_tailcat_process()` helper checking `/proc/$pid/comm` and `/proc/$pid/cmdline` on Linux / Asuswrt firmware before issuing process kill commands.
  * **Immunity to Recycled PIDs:** Prevents long-running sleep watchdogs and teardown routines from accidentally killing unrelated router daemons (e.g. `dnsmasq`, `httpd`, `dropbear`) if a `tailcat` PID rolled over on high-churn router environments.
* **Flash Storage (JFFS) Inode Wear Elimination:**
  * **Stat-Guarded Permission Updates:** Introduced `ensure_file_perm_600()` using `stat` to check existing file permissions before executing `chmod 600`.
  * **Zero Flash Metadata Churn:** Eliminates redundant `ctime` updates and NAND/SPI flash writes on router startup and configuration inspections when permissions are already set to 0600.
* **Phased Auto-Kill Timeout Countdown Warnings:**
  * **Staged Expiration Notices:** Watchdog subshells now proactively broadcast warnings prior to terminating ephemeral sessions:
    * **5-Minute Warning:** Notifies connected operators that session will expire in 5 minutes.
    * **1-Minute Warning:** Emits high-priority alert (`⚠️ Warning: Session will terminate in 60 seconds`).
    * **Expiry:** Broadcasts final termination notice upon session shutdown.
  * **Dual-Channel Delivery:** Dispatches warnings directly to all active host TUI terminals (`/tmp/tailcat_sessions/active_tuis`) and remote guest PTYs (`/dev/pts/*`).
* **WebGUI Localhost / HSTS Browser Guidance:**
  * Added informational notes in session cards and `tailcatzero status` explaining how to handle modern browser SSL domain mismatch or HSTS blocks on `localhost` (by proceeding past self-signed certificates or forwarding the HTTP port).
* **Interactive TUI Runtime Resilience:**
  * Relaxed top-level shell mode from `set -eu` to `set -u`, catching unbound variables while preventing BusyBox `ash` from abruptly crashing interactive TUI sessions on harmless non-zero pipeline returns.
* **Automated Unit Testing:**
  * Expanded [`tests/unit/test_unit_helpers.sh`](tests/unit/test_unit_helpers.sh) with dedicated tests for `ensure_file_perm_600`, `is_tailcat_process`, and `broadcast_session_warning` with 100% test pass rate.


## [1.9.1] - 2026-09-06

### Penetration Hardening, Containment & Denial-of-Service Remediation

* **View-Only Sandbox Penetration Hardening (`src/tailcat-view-shell.c`):**
  * **Dynamic Linker & Environment Sanitization:** At shell entry (`main()`), aggressively unsets dangerous linker, dynamic loader, and execution controls (`LD_PRELOAD`, `LD_LIBRARY_PATH`, `LD_AUDIT`, `LD_DEBUG`, `DYLD_INSERT_LIBRARIES`, `DYLD_LIBRARY_PATH`, `DYLD_FRAMEWORK_PATH`, `BASH_ENV`, `ENV`, `IFS`, `CDPATH`, `GLOBIGNORE`) before launching any child process, completely neutralizing dynamic library hijacking attacks on child `execvp()`.
  * **Network Route & Interface Flush DoS Prevention:** Added `flush`, `save`, and `restore` to the `ip` mutation blacklist. Blocks `ip route flush table main` and `ip addr flush dev eth0`, preventing malicious or accidental routing table wiping and interface disconnects.
  * **Leading Option Parsing in Subcommand Validators:** Upgraded `ip` and `opkg` argument parsers to scan past leading option flags (`-`) to locate the genuine sub-command verb. Safe queries like `ip -4 route`, `ip -6 addr`, `ip -br link`, and `opkg -V 2 list` are now properly supported, while flags placed before mutations (e.g. `ip -4 route flush`) can no longer evade detection.
  * **Broadcom Wireless Radio Shutdown Mitigation:** Added `radio`, `off`, `reinit`, `reset`, `txpower`, `channel`, `ssid`, `wep`, and `wpa` to the `wl` state-changing blacklist, preventing guests from shutting down physical radios (`wl radio off`) or modifying wireless frequencies.
  * **Option-Embedded Sensitive Path & Symlink Traversal:** Created `check_file_path_security()` to inspect full tokens and extract embedded values following `=` or `-f` prefixes (e.g. `--file=/path`, `-f/path`). Symlinks are resolved to canonical destinations via `readlink()` and `realpath()`. Explicitly hard-blocked `sort --files0-from` and `--batch-size`.
  * **Process Memory & Credential Extraction Defense:** Extended `check_sensitive_target()` to block all `/proc/` access targeting `environ`, `mem`, `cmdline`, `fd`, `cwd`, `root`, and `sysrq-trigger`. Prevents dumping process memory or extracting plaintext NVRAM tokens via `/proc/1/environ` or `/proc/self/environ`. Implemented case-insensitive pattern matching (`case_str_search()`) across all sensitive paths and NVRAM credential keys (`cookie`, `session`, `otp`, `totp`, `api`).
  * **Atomic 0600 IPC Request Creation:** Enforced atomic creation of `.req` permission files using `open(req_file, O_WRONLY | O_CREAT | O_EXCL, 0600)`. Escapes quotes, backslashes, `$`, and ``` `` ``` when serializing `RAW_COMMAND` to prevent malformed IPC parsing. Validated tool names against empty strings, `.`, and `..`.
  * **Unclosed Quote & Buffer Length Error Handling:** `split_pipeline()` and `tokenize_stage()` now explicitly reject unclosed single/double quotes or trailing escapes with clear syntax error messages. Input lines exceeding `MAX_LINE_LEN` (4096) are rejected instead of silently truncated.
  * **Pipeline File Descriptor Cleanup & `waitpid` Interruption Handling:** Fixed pipe descriptor leaks on pipeline setup failures, and wrapped `waitpid()` in an `errno == EINTR` loop.
* **Host Approval IPC Hardening (`tailcatzero`):**
  * **Strict Request Attribute Sanitization:** Hardened `parse_request_file()` to sanitize `REQ_ID`, `BASE_CMD`, `GUEST_PID`, and `CREATED_AT` against path traversal and command injection.
* **Security Test Suite Expansion (`tests/unit/test_unit_security.sh`):**
  * Expanded security test suite from 24 to 31 tests covering network flushes, leading flags, wireless radio off, option-embedded symlinks, `/proc/*/mem|environ`, unclosed quotes, and dynamic linker sanitization with a 100% pass rate.


## [1.9.0] - 2026-09-06

### Architectural Evolution: C99 Musl Static View-Only Shell & Complete Shell Trap Elimination

* **Complete C99 Rewrite of `tailcat-view-shell` (`src/tailcat-view-shell.c`):**
  * **Elimination of the Shell Parsing Trap:** Replaced the legacy shell script wrapper with a high-performance, strictly bounded C99 binary. Commands and pipeline stages are tokenized directly into native argument vectors (`argv[]`) without ever invoking `/bin/sh` or `eval`.
  * **Kernel-Level Pipeline Execution:** Multi-stage pipelines (e.g. `uptime | grep -o 'up' | wc -l`) are created directly via POSIX `pipe()`, `fork()`, `dup2()`, and `execvp()` system calls. Unvalidated shell expansions, subshell injections (`$()`, ``` `` ```), file redirections (`>`, `<`), and command chaining (`;`, `&&`, `||`) are impossible by construction.
  * **Zero Dynamic Heap Allocation:** Token buffers, stage arrays, and string canonicalizations operate strictly on fixed, bounded stack and static buffers with compile-time limits (`snprintf`, length bounds), completely eliminating memory leak and buffer overflow attack surfaces.
  * **Musl Libc Static Linking via Zig CC:** Built using `zig cc` targeting Musl libc, creating ultra-lean, self-contained static ELF binaries requiring **zero shared library dependencies** on router firmware:
    * `armv7` (RT-AC68U, RT-AC86U 32-bit compatibility): **132 KB**
    * `arm64` (RT-AX86U, GT-AX6000, modern Wi-Fi 6/6E/7 models): **136 KB**
    * `amd64` (Merlin x86 test environments and virtual routers): **132 KB**
    * `native` (macOS development host): **92 KB**
  * **Built-in Pipeline Support:** Internal commands (`help`, `sysinfo`, `leases`, `wifi`, `ports`, `logs`, `env`, `clear`) can be executed standalone or piped directly into allowed inspection tools with automated standard I/O buffer flushing.
* **Build System & Toolchain Automation:**
  * **`build.sh` & `Makefile`:** Added unified build tooling for cross-compiling static Musl binaries (`make musl`), building native host binaries (`make native`), and executing full test suites (`make test`).
* **Architecture-Aware Installation & Upgrades (`install.sh` & `tailcatzero`):**
  * **Dynamic CPU Architecture Detection:** `get_pkg_arch` detects `armv7`, `arm64`, or `amd64` to download or copy the appropriate precompiled static Musl binary during installation and automatic updates.
  * **Root Dispatcher (`tailcat-view-shell`):** Provided a seamless top-level launcher script for local checkouts that executes the appropriate precompiled binary or automatically compiles native binaries on the fly.
* **100% Security & Regression Parity:**
  * Verified 100% pass rate across all 24 security test suites in [`tests/unit/test_unit_security.sh`](file:///Users/Baris/tailcat-merlin/tests/unit/test_unit_security.sh) and local helper suites.


## [1.8.2] - 2026-09-06

### Penetration Hardening, Sandbox Isolation & Denial-of-Service Mitigations

* **View-Only Sandbox Hardening (`tailcat-view-shell`):**
  * **Sensitive File Request Escalation Defense:** Added `is_sensitive_file_access` validation to `request_command_approval()`. Host requests targeting sensitive system files or credentials (e.g. `request cat /etc/shadow` or pipeline-smuggled `request echo ok | cat /etc/shadow`) are rejected immediately before request creation.
  * **Strict Sensitive File Protection:** Disallowed session-wide tool approvals (`APPROVED_FILE`) from bypassing sensitive file access controls for inspection utilities (`cat`, `sort`, `uniq`, `grep`, `head`, `tail`, `more`, `less`). Sensitive security files remain strictly prohibited in view-only mode.
  * **Symlink Traversal Prevention:** Extended `is_sensitive_file_access()` to inspect all path tokens via `[ -L "$_tok" ]` and resolve canonical destinations using `readlink -f`, preventing direct and multi-hop symlink evasions.
  * **Pipeline `top` Batch Enforcement:** Rebuilt pipeline stage execution into a sanitized pipeline (`sanitized_pipeline`). Any pipeline stage invoking `top` (e.g. `ps | top`) is dynamically rewritten with non-interactive batch flags (`-b -n 1` on Linux, `-l 1 -n 0` on macOS), eliminating interactive process termination (`k`) escapes.
  * **Non-Interactive Pager Streaming:** Implemented `safe_stream()` to filter non-portable flags and stream pager commands (`less`, `more`, `/bin/less`, `/usr/bin/more`) directly through `cat`, preventing interactive pager escapes via explicit binary paths.
  * **Character Device Flood & DoS Mitigation:** Restricted direct reading of raw character devices (`/dev/zero`, `/dev/urandom`, `/dev/random`, `/dev/console`, `/dev/tty*`) and sensitive kernel diagnostic files (`/proc/kmsg`, `/proc/kallsyms`), while preserving `/dev/null`.
  * **Extended NVRAM Credential Blocking:** Expanded `nvram get` credential pattern filters to cover `*[pP][aA][sS][sS]*`, `*[cC][eE][rR][tT]*`, `*[pP][rR][iI][vV]*`, `*[oO][vV][pP][nN]*`, `*[wW][gG]*`, `*[hH][aA][sS][hH]*`, and `*[sS][aA][lL][tT]*` unconditionally.
  * **GNU Option Prefix Evasion Defense:** Broadened option inspection patterns for GNU `diff` (`--diff*`) and `sort` (`--compress*`) to prevent abbreviated flag bypasses.
  * **Unbound Variable Defensiveness:** Defined `VIEW_ONCE_DIR="${SESSIONS_DIR}/VIEW_ONCE"` to prevent `set -u` termination during permission request handling.
* **Service Watchdog & Teardown Parity (`tailcatzero`):**
  * **Watchdog Cleanup Parity:** Enhanced the background timeout watchdog in `start_tunnel()` to clean up `VIEW_APPROVED_FILE`, `VIEW_DENIED_FILE`, `REQUESTS_DIR`, and `VIEW_ONCE_DIR` upon session expiration.
  * **Safeguard Parity:** Included `/dev/mem` and `/dev/kmem` in `resolve_single_request()` and `allow_command_session()` red-line checks.
* **Input Sanitization (`install.sh` & `tailcatzero`):**
  * Sanitized upstream release tags and commit SHAs via `tr -cd 'a-zA-Z0-9_.-'` before URL interpolation.


## [1.8.1] - 2026-09-06

### Security Hardening, Privilege Escalation Prevention & Repository Standardization

* **View-Only Sandbox Filter Evasion Hardening (`tailcat-view-shell`):**
  * **Input Canonicalization:** Strips quote combinations and backslash escapes before validation, preventing blacklist filter evasion.
  * **Wildcard Glob Inspection:** Expands and inspects file path globs against sensitive system files (`/etc/shadow`, `/etc/passwd`), blocking glob-based credential dumps (`cat /etc/pas*`, `grep root /tmp/etc/sha*`).
  * **Hardware & Memory Device Node Protection:** Restricted direct access to raw flash partitions, memory nodes, and block devices (`/dev/mtd*`, `/dev/mem`, `/dev/kmem`, `/dev/port`, `/proc/kcore`). Added `/dev/mem` and `/dev/kmem` to the non-escalatable permanent blocklist.
  * **Multi-Variable NVRAM Query Filtering:** Iterates and validates every variable token in multi-argument queries (`nvram get <var1> <var2>`), preventing credential extraction via trailing arguments. Added account lists (`acc_list`, `acc_webdavusers`) and certificates (`*crt*`) to restricted targets.
  * **In-Tool Flag Inspections:** Prohibits destructive or execution flags across diagnostic utilities (`tree -o`, `sort -o`, `diff --diff-program`, `dmesg -c`, `ping -f`, `date -s`).
  * **Tool Coverage:** Extended sensitive file checks across `stat`, `file`, and `uniq`.
  * **Operator Prohibition:** Disallowed background process execution (`&`) alongside command chaining.
* **Host IPC & Parser Security (`tailcatzero`):**
  * **Deterministic Key-Value Parsing:** Replaced shell sourcing (`. "$f"`) of guest request files (`.req`) and session state files (`.env`) with safe `IFS='=' read -r` line parsers, eliminating command injection risks during host approval and status inspections.
  * **Permission Hardening:** Enforced `chmod 700` on session, request, and active TUI directories, and `chmod 600` on configuration and address files.
* **Repository & Installer Standardization:**
  * **Repository Cleanup:** Removed redundant legacy `tailcat` script from the repository root, standardizing exclusively on `tailcatzero`.
  * **Installer Defensiveness (`install.sh`):** Defined `C_YELLOW` to prevent unbound variable abort under `set -u` during failed GitHub lookups, and removed legacy local `./tailcat` fallback logic.


## [1.8.0] - 2026-09-05

### 📥 Default Recursive Drop Box, Hash-Based Auto-Updates & Namespace Protection

* **📥 Recursive Folder Support by Default (`--accept-dirs`):**
  * Configured `tailcatzero recv` (both interactive TUI Option 2 and CLI `tailcatzero recv [dir]`) to launch with `--accept-dirs` by default.
  * Senders can now upload entire directory trees (`tailcat cp -r folder/ <token>:`) as well as single files, while preserving original filenames, subfolders, and hierarchy.
  * Verified byte-for-byte fidelity across recursive directory trees in automated integration tests.
* **⚡ Hash & MD5-Based Update Detection:**
  * Replaced version-only update checks in `update_tailcat_all` with cryptographic hash (`md5sum`) comparison.
  * Upstream fixes, patches, and minor improvements are detected and applied even when the SemVer tag remains unchanged.
  * Eliminates redundant flash writes to `/jffs` when the local script is byte-for-byte identical to upstream.
  * Added `tailcatzero check-update` (or `tailcatzero check`) for non-destructive upstream update checking with status reporting.
  * Enhanced `tailcatzero -v` / `--version` to output the active script's short hash alongside SemVer and engine version.
* **🛡️ View-Only Sandbox Dynamic Approvals & Proactive Permissions:**
  * Added `tailcatzero allow <cmd>` to proactively grant command permissions in the active view-only session.
  * Added `tailcatzero revoke <cmd>` to revoke permissions from the active view-only allowlist.
  * Added `tailcatzero approve [id|cmd] [--once]` supporting single-use execution authorization (`--once`) alongside session-wide approvals.
* **📦 Namespace Protection & Compatibility:**
  * Removed all `tailcat` command-name symlinking/hijacking across the installer and CLI to protect the official `tailcat` Go package namespace. Standardized strictly on `tailcatzero`.


## [1.7.1] - 2026-09-05

### 🛡️ Security Hardening, Privilege Escalation Prevention & Zero-Config WebGUI Forwarding

* **🛡️ View-Only Sandbox Hardening (`tailcat-view-shell`):**
  * **Direct Shell Escape Neutralization:** Removed `env` from allowlist. Built-in `env`/`printenv` now strictly displays variables; executing commands via `env <cmd>` is rejected with a security violation error.
  * **Interactive Pager Breakout Neutralization:** Enforced `LESSSECURE=1` in environment and wrapped `less` / `more` to stream safely via `cat`, permanently eliminating `:!sh` breakout vectors.
  * **In-Tool File Overwrite Prevention:** Removed `xxd` from allowlist (`hexdump` remains available). Filtered `sort` to block file-writing (`-o`) and compressor execution (`--compress-program`). Filtered `uniq` to prohibit positional output destination arguments (`uniq [in [out]]`).
  * **Network & Interface Mutation Prevention:** Added verb validation preventing mutating operations across `route` (`add`, `del`), `arp` (`-s`, `-d`, `-f`), and `wl` (`down`, `up`, `join`, `set`, `restart`).
  * **Credential & Secret Extraction Protection:** Blacklisted access to sensitive security paths (`/etc/shadow`, `/tmp/etc/shadow`, `.ssh/id_*`, `dropbear`, `.key`) across all inspection tools. Blocked plaintext credential dumps via `nvram show` and filtered credential keys in `nvram get` (`*passwd*`, `*psk*`, `*secret*`, `*key*`).
  * **Multi-line / Control Character Injection Block:** Rejects carriage returns (`\r`) and newlines (`\n`) in input strings before parsing.
  * **GTFOBin Threat Detection for Host Approvals:** Added automated identification and high-visibility warning banners across TUI and CLI when a guest requests permission for tools capable of executing subshells or modifying files (`awk`, `find`, `python`, `sed`, `tar`, etc.).
* **🌐 Zero-Config WebGUI Forwarding (`tailcat forward`):**
  * Replaced client SOCKS proxy guidance with native `tailcat forward <TOKEN> <port>`, providing instant direct browser access via `https://localhost:<port>` without manual SOCKS browser configuration.
* **📦 Core Script Harmonization:**
  * Renamed repository root script from `tailcat` to `tailcatzero` to match the router installation target and CLI binary name.


## [1.7.0] - 2026-09-05

### 🔔 Interactive On-Demand Permission Escalation for View-Only Sessions

* **🔔 On-Demand Permission Escalation (`tailcat-view-shell`):** Remote support technicians and friends in view-only sessions can request execution authorization for additional diagnostic tools via `request <command>` (or `req <cmd>` / `ask <cmd>`), or simply by answering `y` when prompted upon typing a restricted command.
* **⚡ Dual-Channel Host Approval Engine:**
  * **Interactive TUI Dashboard:** Real-time notification badge (`🔔 [%d Pending Requests - Press P to Review]`) displayed on the main menu and active session cards. Pressing `[P]` opens an interactive modal to review, inspect full commands, and approve or deny.
  * **Headless CLI Interface:** Router administrators can manage requests entirely via CLI without opening the TUI:
    * `tailcatzero requests`: Lists all pending requests with tool, full command line, and age.
    * `tailcatzero approve [id|cmd] [--once]`: Approves the request for the session (default) or single execution (`--once`).
    * `tailcatzero deny [id|cmd]`: Denies the request and adds the base command to the session blocklist to suppress repeat prompts.
    * `tailcatzero allow <cmd>`: Proactively adds a command to the session allowlist before it is even requested.
    * `tailcatzero revoke <cmd>`: Removes a command from the session allowlist.
* **🎯 Granular Approval Scopes:**
  * **Session-Wide (`APPROVED_SESSION`):** Adds the base command to volatile RAM (`/tmp/tailcat_sessions/VIEW_APPROVED.txt`), allowing the guest to run the tool repeatedly for the remainder of the session without re-prompting.
  * **Single Execution (`APPROVED_ONCE`):** Grants single-use authorization for the pending command instance only.
* **🛡️ Hardened Flash & Partition Brick Protection:** Critical low-level operations that could permanently brick or corrupt flash partitions (`dd of=/dev/mtd*`, `flash_erase*`, `rm -rf /`, `nvram erase`) are hard-blocked from ever being requested or authorized.
* **📢 Real-Time Admin Broadcasts:** Automatically notifies the host router admin via syslog (`tailcatzero`) and broadcasts alert banners across active SSH terminals (`/dev/pts/*`).


## [1.6.0] - 2026-09-04

### 🔒 Restricted View-Only Diagnostic Shell, Entware Inspection & 5-Slot Multi-Service Concurrency

* **🔒 Restricted View-Only Diagnostic Shell (`tailcat-view-shell`):** Introduced a zero-trust, read-only remote support shell option (`Option 1 -> 2` or `tailcatzero view` / `tailcatzero ssh view`). Clients connect using standard `tailcat ssh <token>` while the session is strictly restricted to safe diagnostic inspection.
* **🛡️ Hardened Multi-Layer Security Sandbox:** Prohibits file redirections (`>`, `>>`, `<`), subshells (`` ` `` / `$()`), command chaining (`;`, `&&`, `||`), state-modifying binaries (`rm`, `mv`, `cp`, `touch`, `chmod`, `dd`), router configuration changes (`nvram set/commit/unset`, `reboot`, `kill`), and package mutations (`opkg install/remove/upgrade`).
* **📦 Deep Entware & Asuswrt Diagnostics:** Permits comprehensive read-only tools across system health (`uptime`, `free`, `df`, `ps`, `top`, `dmesg`, `sysinfo`), networking & WiFi (`ip addr/route`, `netstat`, `route`, `ping`, `mtr`, `wl`, `leases`, `wifi`, `ports`), NVRAM queries (`nvram get`, `nvram show`), text processing (`cat`, `head`, `tail`, `grep`, `rg`, `tree`, `sort`, `uniq`, `diff`), and Entware queries (`opkg list/info/find/status/search/depends`).
* **⚡ Safe Pipeline Execution:** Supports Unix pipelines (`|`) between allowed tools (e.g. `ps | grep dnsmasq`, `nvram show | grep dhcp`, `opkg list-installed | grep python`).
* **🆘 Remote Support Shell Options Menu (Option 1):** Main dashboard Option 1 opens a clean submenu allowing admins to choose between `1. Full Root Shell (Read-Write)` and `2. View-Only Diagnostic Shell (Read-Only)`. Features dynamic visual badge indicators (`[🟢 Root + 🔒 View]`).
* **🖐️ 5-Slot Multi-Service Concurrency:** Upgraded active session tracking, multi-session card overview, and selective kill confirmations to support 5 concurrent service slots (`svc_1` to `svc_5`).


## [1.5.0] - 2026-09-04

### 🚀 CLI Timeout Management, Multi-Arch (`amd64`), DERP_URL Preservation & Self-Healing Resilience

* **⏱️ CLI Timeout Management (`tailcatzero timeout`):** Direct command-line inspection and configuration of the auto-kill timeout (`tailcatzero timeout [min|persistent]`) without entering interactive TUI menus. Supports `persistent` or `0` for persistent mode.
* **🛡️ DERP Map URL Preservation:** Implemented `save_timeout_config` helper to preserve existing `DERP_URL` definitions in `/jffs/addons/tailcat/tailcat.cfg` when updating session timeouts.
* **💻 Universal Architecture Expansion (`amd64` / `x86_64`):** Added 64-bit x86 architecture detection to both `install.sh` and `update_binary`, downloading official `tailcat_linux_amd64` binaries for x86-based Asuswrt-Merlin environments, QEMU, and container testbeds.
* **🩹 Self-Healing CLI Dependency Installation:** Invoking tunnel subcommands (`ssh`, `view`, `recv`, `files`, `webgui`) on systems missing the Go engine binary or view shell automatically downloads and sets up dependencies on-the-fly.
* **🧹 Self-Healing Dead Process & Watchdog Cleanup:** Enhanced `is_service_active` to detect externally terminated processes, auto-remove stale `.env` files and address dumps, and terminate orphaned watchdog subshells.
* **🌐 Accurate Protocol & Port Extraction:** Unified `get_webgui_connect_info` across multi-session overview, single-session card, and `tailcatzero status` for seamless HTTP (port 80) and HTTPS (custom ports / 8443) WebGUI access.
* **🗑️ Comprehensive Uninstaller Hardening:** Added removal of legacy `/opt/bin/tailcat` symlinks, address files, and POSIX case-insensitive `/jffs/scripts/init-start` cleanup.


## [1.4.0] - 2026-09-03

### 🚀 Rebranding to TAILCAT ZER0, Non-Interactive CLI Interface (`tailcatzero`), Persistent Mode & Dual Self-Updater

* **🐱 Project Rebranding to TAILCAT ZER0:** Officially rebranded to **TAILCAT ZER0** with the dedicated CLI command `tailcatzero`.
* **🖥️ Dedicated Non-Interactive CLI Dispatcher:** Full command-line interface accessible via `tailcatzero` across `/jffs/scripts/` and `/opt/bin/`. Supports subcommands (`status`, `stop [all|SVC]`, `ssh`, `webgui`, `update`, `-v`, `-h`) for headless automation and non-interactive SSH sessions.
* **⏱️ Persistent Mode & Custom Timeout:** Users can specify any custom auto-kill duration in minutes or enter `0` (or `persistent`) for persistent tunnels that run until manually stopped without spawning background sleep watchdog processes.
* **🔁 Interactive In-Place Input Validation:** Timeout configuration now features an interactive retry loop that reprompts on non-digit or invalid input without dropping the user back to the main menu.
* **🔄 Dual Script & Engine Self-Updater:** Option 6 (`manage_tailcat_menu`) and `tailcatzero update` fetch both the latest CLI script from GitHub (`underd0se/tailcat-zero`) and the official Go engine binary for the router architecture.
* **🔑 Guaranteed Ephemeral Tokens (`--key=new`):** Enforces `--key=new` across all tunnel spawns so every session generates a fresh, unique cryptographic WireGuard keypair and address token, preventing token reuse.
* **📋 Streamlined 6-Item Menu:** Consolidated configuration, update, reinstall, and uninstall options into a dedicated management submenu with active status badges.


## [1.3.0] - 2026-09-02

### 🚀 Multi-Service Concurrency, Side-by-Side ASCII Cat Header & Interactive TUI Management

* **⚡ Multi-Service Concurrent Management:** Full concurrent execution and management across all 4 services (Remote Shell, File Receiver, SFTP Directory Share, and Router WebGUI). Multiple tunnels can run simultaneously with independent WireGuard userspace nodes, tokens, and watchdog auto-kill timers in `/tmp/tailcat_sessions/`.
* **🐱 Side-by-Side ASCII Cat Header:** Compact, elegant side-by-side Japanese ASCII cat (`╱|、`) and project title/description layout.
* **⚙️ Dedicated Management Submenu:** Interactive management menu allowing users to update the Go engine binary, perform a fresh reinstall, or execute a complete uninstallation with clean init hook removal.
* **🛑 Selective & Batch Process Killer:** Stop action (`S`) lists all running services with PIDs and remaining time, allowing users to stop specific individual processes or all tunnels simultaneously.
* **✨ Flicker-Free Clean Canvas & Toast Banners:** Integrated VT100 screen-clearing (`clear_screen`) and transient `FLASH_MSG` toast banners across all menus and cancellations to prevent dirty terminal scrolling.
* **🔤 Natural Underlined Hotkey Styling:** Clean single-character underlined hotkeys (`<u>V</u>iew Sessions | 🛑 <u>S</u>top | ↩️ <u>E</u>xit: `) without redundant `=` symbols.


## [1.2.0] - 2026-09-02

### 🚀 TUI Dashboard, Dynamic Storage Awareness & Smart Status Badges

* **📺 Flicker-Free Live Dashboard:** Screen cleanly refreshes without terminal history scroll clutter when updating live timers (`r`) or toggling ASCII QR codes (`q`).
* **💾 Dynamic Inbox Storage Awareness:** Displays real-time free disk space for volatile RAM (`/tmp`) and mounted USB partitions (`/tmp/mnt/*`), with dynamic choice numbering based on USB presence.
* **🌐 Browser-Ready WebGUI Guidance:** Generates step-by-step zero-configuration instructions for remote administrators connecting via TCP port forwarding (`1. Run: tailcat forward <token> <port>`, `2. Open browser: https://localhost:<port>`).
* **🟢 Smart Status Indicators & Menu Badging:** Added prominent visual status badge (`🟢 ACTIVE` vs `⚪ INACTIVE`) and dynamically badges menu options 5 & 6 with active timer and kill labels.
* **⏱️ Precision Auto-Kill Warning:** Displays `< 1m remaining (expiring soon)` when session timer falls below 60 seconds.


## [1.1.3] - 2026-09-02

### ↩️ Submenu Navigation & Cancellation Support

* **↩️ Submenu Cancel Navigation:** Added full support for canceling and returning to the main menu using `e` / `b` / `cancel` from Inbox destination selection, SFTP directory/mode prompts, and Auto-Kill timeout configuration, preventing accidental tunnel launches.


## [1.1.2] - 2026-09-02

### 🧹 UI Cleanup, Anti-Bleed Dividers & Underline Hotkeys

* **🧹 Active Session Header:** Renamed card header to concise `Active Session`.
* **✂️ Cleaner Status Details:** Removed redundant explanations from Auto-Kill and Security metadata rows.
* **🛡️ Anti-Bleed Snippet Dividers:** Replaced fixed-width closed boxes with horizontal rule dividers (`───`) so long tokens and commands naturally flow without line-wrap border corruption.
* **📁 Folder Icon Alignment:** Updated SFTP Directory Sharing icon to `📁` (Folder) for improved visual metaphor.


## [1.1.1] - 2026-09-02

### 🎨 High-Contrast Terminal Color Refinements

* **🎨 Enhanced Dark-Theme Readability:** Upgraded Chat Invite Snippets to high-contrast crisp white (`C_WHITE`) and cyan borders (`C_CYAN`) with highlighted yellow commands (`C_YELLOW`), ensuring pristine visibility across dark-background terminals (Ghostty, iTerm2, macOS Terminal).


## [1.1.0] - 2026-09-02

### ✨ Modern UX, Live Session Card & KISS Architecture Refactor

* **📱 Live Active Session Card:** Dedicated interactive dashboard displaying live auto-kill countdown, active service details, and single-key actions (`[s] Stop`, `[r] Refresh`, `[q] QR Code`, `[b] Back`).
* **📷 Integrated ASCII QR Codes:** Direct rendering of ASCII QR codes in terminal via router's built-in `qrencode` for rapid mobile/tablet token capture.
* **💬 Ready-to-Paste Chat Snippets:** Generates pre-formatted 2-line invite text ready to copy-paste into Discord, Slack, or WhatsApp.
* **⚡ Global Contextual Hotkeys:** Single-key controls across the menu (`s` to stop immediately, `t` for timeout, `v` for session card).
* **🎯 KISS Feature Alignment:** Streamlined menu to the 4 core sharing pillars (Shell, File Receiver, SFTP, WebGUI), eliminating unnecessary feature creep.
* **💾 Dynamic USB Storage Detection:** Automatically offers mounted USB partitions (`/tmp/mnt/*`) for Inbox storage to avoid RAM exhaustion.


## [1.0.0] - 2026-09-02

### 🚀 Initial Release: Ephemeral WireGuard Tunnel & File Receiver Manager

* **🆘 Instant Remote Shell (Passwordless):** Ephemeral WireGuard shell powered by TailCat's native SSH server with capability-based token access (no passwords or SSH keys to configure).
* **📥 Encrypted File Receiver:** Write-only peer-to-peer file drop receiver into `/tmp/tailcat-inbox`.
* **📤 SFTP File Share:** Read-only directory serving with native SFTP path confinement.
* **🌐 WebGUI Remote Port Forwarder:** Securely forward local router management WebUI (port 8443).
* **⏱️ Automated 30-Minute Session Auto-Kill:** Background supervisor process automatically tears down active sessions when timer expires.
* **📦 Universal ARMv7 & ARM64 Architecture Support:** Automatically fetches official `tailscale/tailcat` `v0.4.0` static binaries for all Asuswrt-Merlin routers.
