# 📥 P2P File Transfers & Inbox

TAILCAT ZER0 transforms your Asuswrt-Merlin router into a secure, peer-to-peer file transfer hub. You can upload large firmware files and scripts directly to the router or share local directories with remote clients using native SFTP—all over encrypted WireGuard without opening WAN ports or configuring FTP daemons.

---

## 📥 1. Encrypted File Receiver (`recv`)

The **Encrypted File Receiver** creates an ephemeral one-way inbox on your router. Any client with the capability token can upload individual files or recursive folder hierarchies directly to this inbox.

By default, TAILCAT ZER0 starts the receiver with `--accept-dirs`, accepting both single files and full directory trees (`tailcat cp -r`) while preserving original filenames and directory structures.

### Common Use Cases:
* Uploading custom firmware (`.trx` / `.pkg`) or rescue images directly to the router.
* Transferring entire script directories, custom themes, or package folders.
* Transferring JFFS backup archives (`jffs_backup.tar.gz`) for restoration.
* Transferring large script packages or diagnostic bundles without setting up SCP or entering passwords.

### Starting the File Receiver:

#### Via Interactive TUI:
1. Run `tailcatzero` ➔ select **Option 2 (📥 Receive Files)**:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  📥 Select Inbox Destination:

  1. ⚡ Volatile RAM:  /tmp/tailcat-inbox           (Free: 384.2M) [Default]
  2. 💾 USB Storage:   /tmp/mnt/USB/tailcat-inbox   (Free: 28.4G) [Persistent]
  3. 📂 Custom Directory Path

========================================================================

  ⚡ RAM (Default)  |  💾 USB  |  📂 Custom  |  ↩️ Back: 
```

2. Select your destination:
   * **RAM (`r` / `1` / Enter):** Fast, volatile inbox (`/tmp/tailcat-inbox`). Disappears cleanly on reboot.
   * **USB (`u` / `2`):** Persistent storage on your mounted USB drive (`/tmp/mnt/USB/tailcat-inbox`).
   * **Custom (`c` / `3`):** Prompts for any custom directory path.

3. The File Receiver Active Session Card renders your upload command and connect token:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🐱 TAILCAT ZER0 — Active Session
========================================================================

  Service:     📥 Encrypted File Receiver
  Destination: /tmp/tailcat-inbox
  Auto-Kill:   ⏱️ 30m remaining
  Security:    🔒 WireGuard P2P Encrypted (Zero WAN Ports Open)

  💬 Copy & Paste to Friend / Admin Support:
  ────────────────────────────────────────────────────────────────────────
  Hey, I've opened a temporary TailCat session on my router (30m remaining).
  Run: tailcat cp <file_or_dir> tcpGFwWCCGsJ9JQ9WPomu5WUUGZY-kf26El-WSoPKoaeXo2RSGdGFrWCAsP6FxwhlwsFaNAiWcV6Ryp27eR4ho4IFCASRQCZRcRGFxWCAcb6A83f_08x3mY0kpBKLax4Uj_dP81xxlOF5T9ARnRmFpGQEv:
  ────────────────────────────────────────────────────────────────────────

========================================================================

  🛑 Stop  |  📱 QR Code  |  🔄 Refresh  |  ↩️ Back: 
```

#### Via CLI:
```sh
# Start with default inbox (/tmp/tailcat-inbox)
tailcatzero recv

# Start with a custom destination directory
tailcatzero recv /tmp/mnt/USB_DRIVE/inbox
```

### Uploading Files from Client Machine:

#### Client Terminal: Uploading Firmware to Router Inbox
```text
┌──(user@laptop)-[~/Downloads]
└─$ tailcat cp RT-AX86U_3004_388.8_2.trx tcpGFwWCCGsJ9JQ9WPomu5WUUGZY...:
[+] Connecting to WireGuard peer via DERP relay (fra)...
[+] Direct WireGuard connection established (UDP 192.168.50.1:51820)
[+] Transferring: RT-AX86U_3004_388.8_2.trx (64.2 MB) -> /tmp/tailcat-inbox/
    64.2 MB / 64.2 MB [======================================] 100% 12.8 MB/s
[✓] Transfer complete: 67,318,528 bytes received in 5.0s
```

#### Router Terminal: Verifying Received File
```text
admin@RT-AX86U:/tmp/home/root# ls -lh /tmp/tailcat-inbox/
total 65744
-rw------- 1 admin root 64.2M Sep  5 22:18 RT-AX86U_3004_388.8_2.trx
```

#### Client Terminal: Uploading Entire Folder Hierarchy (`-r`)
Because `--accept-dirs` is enabled by default, senders can upload full folder trees preserving subdirectories and filenames:

```text
┌──(user@laptop)-[~/Projects]
└─$ tailcat cp -r my_scripts/ tcpGFwWCCGsJ9JQ9WPomu5WUUGZY...:
[+] Connecting to WireGuard peer via DERP relay (fra)...
[+] Direct WireGuard connection established (UDP 192.168.50.1:51820)
[+] Transferring: my_scripts/ -> /tmp/tailcat-inbox/my_scripts/
[✓] Directory tree transferred successfully
```

#### Router Terminal: Verifying Directory Structure
```text
admin@RT-AX86U:/tmp/home/root# ls -laR /tmp/tailcat-inbox/my_scripts/
/tmp/tailcat-inbox/my_scripts:
drwxr-xr-x    3 admin    root            80 Sep  5 22:20 .
drwxr-xr-x    2 admin    root            60 Sep  5 22:20 utils
-rw-r--r--    1 admin    root          1024 Sep  5 22:20 run.sh
```

> [!TIP]
> RAM vs USB: The default `/tmp/tailcat-inbox` directory resides in router RAM. If transferring files or directories larger than your available free RAM (`free -m`), specify a mounted USB disk path instead!

---

## 📁 2. SFTP Directory Sharing (`files`)

The **SFTP Directory Sharing** feature exposes a specified router directory to remote clients over TailCat's internal SFTP server.

### Sharing Modes:
* **Read-Only (`ro`) - Recommended:** Remote clients can list, view, and download files, but cannot create, modify, or delete anything.
* **Read-Write (`rw`):** Remote clients have full bi-directional write and delete access to the shared folder.

### Starting SFTP Sharing:

#### Via Interactive TUI:
1. Run `tailcatzero` ➔ select **Option 3 (📁 Share Directory - SFTP)**:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  📁 Select Directory to Share (SFTP):

  1. 📂 Default JFFS:  /jffs                         (Router configs) [Default]
  2. 💾 USB Storage:   /tmp/mnt/USB                  (Mounted storage)
  3. 📂 Custom Directory Path

========================================================================

  📂 JFFS (Default)  |  💾 USB  |  📂 Custom  |  ↩️ Back: 
```

2. Select permission mode (`ro` or `rw`):

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🔒 SFTP Permission Mode for /jffs:

  1. 🔒 Read-Only (ro)      Safe remote browsing; no edits or deletes [Default]
  2. ✏️  Read-Write (rw)     Full remote upload, edit, and delete access

========================================================================

  🔒 Read-Only (Default)  |  ✏️ Write  |  ↩️ Back: 
```

3. The SFTP Active Session Card displays your connection token and `tailcat ls` command:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🐱 TAILCAT ZER0 — Active Session
========================================================================

  Service:     📁 SFTP File Share (/jffs)
  Destination: Path: /jffs (ro)
  Auto-Kill:   ⏱️ 30m remaining
  Security:    🔒 WireGuard P2P Encrypted (Zero WAN Ports Open)

  💬 Copy & Paste to Friend / Admin Support:
  ────────────────────────────────────────────────────────────────────────
  Hey, I've opened a temporary TailCat session on my router (30m remaining).
  Run: tailcat ls -l tcpGFwWCBwoiH7ENLsCVJqJkf3bqslXzPvZN8ojZE1xhLsIh1SNmFrWCBCSVSvDt58ByyZZ59rFCypZTzDrgIWPbSv7g7USo5mI2FxWCBLJkmyTLROiWxunuYi66TFGbl7na4F1KbxXpMxBHdbL2FpGQEv
  ────────────────────────────────────────────────────────────────────────

========================================================================

  🛑 Stop  |  📱 QR Code  |  🔄 Refresh  |  ↩️ Back: 
```

#### Via CLI:
```sh
# Share /jffs as read-only (default)
tailcatzero files /jffs ro

# Share a USB drive folder as read-write
tailcatzero files /tmp/mnt/USB_DRIVE/media rw
```

### Interacting from Client Machine:

#### Client Terminal: Listing Remote Router Directory
```text
┌──(user@laptop)-[~]
└─$ tailcat ls -l tcpGFwWCBwoiH7ENLsCVJqJkf3bqslXzPvZN8ojZE...
[+] Connecting to WireGuard peer via DERP relay (fra)...
[+] Direct WireGuard connection established (UDP 192.168.50.1:51820)
drwxr-xr-x   admin   root          0 Sep  4 12:00 configs
drwxr-xr-x   admin   root          0 Sep  4 12:00 scripts
-rw-r--r--   admin   root        412 Sep  4 12:05 openvpn-up.sh
-rw-------   admin   root       1024 Sep  1 09:30 tailcat.cfg
```

#### Client Terminal: Downloading Files from Router
```text
┌──(user@laptop)-[~]
└─$ tailcat cp tcpGFwWCBwoiH7ENLsCVJqJkf3bqslXzPvZN8ojZE...:scripts/openvpn-up.sh ./openvpn-up.sh
[+] Connecting to WireGuard peer via DERP relay (fra)...
[+] Direct WireGuard connection established (UDP 192.168.50.1:51820)
[+] Downloading scripts/openvpn-up.sh (412 B) -> ./openvpn-up.sh
    412 B / 412 B [==============================================] 100% 1.2 MB/s
[✓] Transfer complete: 412 bytes received in 0.04s
```

#### Client Terminal: Uploading Files (when in `rw` mode)
```text
┌──(user@laptop)-[~]
└─$ tailcat cp custom_script.sh tcpGFwWCBwoiH7ENLsCVJqJkf3bqslXzPvZN8ojZE...:scripts/
[+] Connecting to WireGuard peer via DERP relay (fra)...
[+] Direct WireGuard connection established (UDP 192.168.50.1:51820)
[+] Uploading custom_script.sh -> scripts/
[✓] Transfer complete: 2,048 bytes sent in 0.1s
```

---

## 🛑 Stopping File Sharing

When your file transfer completes, terminate the service:
```sh
tailcatzero stop RECV    # Stop File Receiver
tailcatzero stop FILES   # Stop SFTP Share
```
All active connections are dropped and capability tokens are immediately invalidated.
