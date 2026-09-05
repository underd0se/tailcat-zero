# 📥 P2P File Transfers & DropBox

TAILCAT ZER0 transforms your Asuswrt-Merlin router into a secure, peer-to-peer file transfer hub. You can upload large firmware files and scripts directly to the router or share local directories with remote clients using native SFTP—all over encrypted WireGuard without opening WAN ports or configuring FTP daemons.

---

## 📥 1. Encrypted DropBox Receiver (`recv`)

The **DropBox Receiver** creates an ephemeral one-way inbox on your router. Any client with the capability token can upload files directly to this inbox.

### Common Use Cases:
* Uploading custom firmware (`.trx` / `.pkg`) or rescue images directly to the router.
* Transferring JFFS backup archives (`jffs_backup.tar.gz`) for restoration.
* Transferring large script packages or diagnostic bundles without setting up SCP or entering passwords.

### Starting DropBox:

#### Via Interactive TUI:
1. Run `tailcatzero` ➔ select **Option 2 (📥 Receive Files)**:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  📥 Select DropBox Destination:

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

3. The DropBox Active Session Card renders your upload command and connect token:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🐱 TAILCAT ZER0 — Active Session
========================================================================

  Service:     📥 Encrypted File DropBox
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
On any PC, laptop, or remote machine with `tailcat` installed:

```sh
# Upload a firmware image to the router's inbox
tailcat cp RT-AX86U_3004_388.8_2.trx tcXXXXXXXXXXXX:

# Upload a backup archive
tailcat cp backup.tar.gz tcXXXXXXXXXXXX:
```

Once the transfer finishes, files are immediately available on the router:
```sh
ls -lh /tmp/tailcat-inbox/
```

> [!TIP]
> RAM vs USB: The default `/tmp/tailcat-inbox` directory resides in router RAM. If transferring files larger than your available free RAM (`free -m`), specify a mounted USB disk path instead!

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

#### 1. Listing Directory Contents
```sh
tailcat ls tcXXXXXXXXXXXX
```

#### 2. Downloading Files from Router
```sh
# Download a configuration backup to your local computer
tailcat cp tcXXXXXXXXXXXX:jffs/configs/dnsmasq.conf.add ./dnsmasq.conf.add

# Download the router syslog
tailcat cp tcXXXXXXXXXXXX:tmp/syslog.log ./router_syslog.log
```

#### 3. Uploading Files (in `rw` mode)
```sh
tailcat cp custom_script.sh tcXXXXXXXXXXXX:jffs/scripts/
```

---

## 🛑 Stopping File Sharing

When your file transfer completes, terminate the service:
```sh
tailcatzero stop RECV    # Stop DropBox
tailcatzero stop FILES   # Stop SFTP Share
```
All active connections are dropped and capability tokens are immediately invalidated.
