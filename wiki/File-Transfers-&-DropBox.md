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
1. Run `tailcatzero`.
2. Select **Option 2 (📥 Receive Files)**.
3. Choose inbox destination:
   * **Option 1:** Default volatile inbox (`/tmp/tailcat-inbox` in RAM).
   * **Option 2:** Custom directory (e.g. `/tmp/mnt/USB_DRIVE/inbox` on an external hard drive).

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
1. Run `tailcatzero`.
2. Select **Option 3 (📁 Share Directory - SFTP)**.
3. Select folder to share (default `/jffs`) and permission level (`ro` or `rw`).

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
