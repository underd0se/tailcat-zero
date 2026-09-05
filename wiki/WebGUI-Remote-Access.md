# 🌐 WebGUI Remote Access

**TAILCAT ZER0** lets you access your Asuswrt-Merlin router's graphical administration interface (WebGUI) from anywhere in the world—even from behind a strict CGNAT, mobile hotspot, or double-NAT—without opening a single WAN port on your router.

---

## ⚠️ The Traditional Problem: WAN WebGUI Insecurity

Historically, managing an Asuswrt router remotely required enabling **"Web Access from WAN"** in *Administration ➔ System*. This exposes the router's web server (`httpd`) directly to port scanners, brute-force bots, and zero-day vulnerabilities across the public internet.

Traditional VPN setups (OpenVPN or standard WireGuard) require:
* A public, routable WAN IPv4 address.
* Dynamic DNS (DDNS) setup.
* Opening UDP ports on your WAN firewall.
* Exporting, distributing, and importing `.ovpn` or `.conf` files onto client devices.

---

## 🔒 The TAILCAT ZER0 Approach: Zero-Config Port Forwarding

TAILCAT ZER0 creates an ephemeral encrypted WireGuard tunnel targeting your router's internal loopback interface (`127.0.0.1:8443` or `127.0.0.1:80`).

```text
  [Remote Laptop]                                       [Asuswrt Router]
+-------------------+                                 +-------------------+
|  Web Browser      |                                 |                   |
|  https://         |                                 |                   |
|  localhost:8443   |                                 |                   |
+---------+---------+                                 |                   |
          |                                           |                   |
          v                                           |                   |
+-------------------+      P2P WireGuard Tunnel       +-------------------+
|  tailcat forward  | ==============================> |  TailCat Engine   |
|  <TOKEN> 8443     |     (Encrypted via DERP)        |      (Local)      |
+-------------------+                                 +---------+---------+
                                                                |
                                                                v
                                                      +-------------------+
                                                      | Asuswrt WebGUI    |
                                                      | 127.0.0.1:8443    |
                                                      +-------------------+
```

* **No WAN ports opened:** Completely invisible to internet port scanners (Shodan, Censys).
* **CGNAT / 4G / 5G Friendly:** Works behind Starlink, mobile hotspots, and CGNAT ISPs.
* **Ephemeral:** Automatically tears down when your session timer expires.

---

## 🚀 How to Use WebGUI Remote Access

### Step 1: Start WebGUI Tunnel on Router

#### Via Interactive TUI:
1. Run `tailcatzero`.
2. Select **Option 4 (🌐 Expose Router WebGUI)**.
3. TAILCAT ZER0 automatically inspects NVRAM (`https_lanport` / `http_lanport`), binds the tunnel, and renders the dedicated WebGUI Active Session Card:

```text
  TAILCAT ZER0 v1.7.1              ╱|、
                                 (˚ˎ 。7
                                  |、˜〵
  Instant Tunnel Manager         じしˍ,)ノ

========================================================================

  🐱 TAILCAT ZER0 — Active Session
========================================================================

  Service:     🌐 Router WebGUI (Port 8443)
  Destination: https://192.168.50.1:8443 (Ports: 8443,80)
  Auto-Kill:   ⏱️ 30m remaining
  Security:    🔒 WireGuard P2P Encrypted (Zero WAN Ports Open)

  💬 Copy & Paste to Friend / Admin Support:
  ────────────────────────────────────────────────────────────────────────
  Hey, I've opened a temporary TailCat session on my router (30m remaining).
  1. Run: tailcat forward tcpGFwWCBVZX0... 8443
  2. Open browser: https://localhost:8443
  ────────────────────────────────────────────────────────────────────────

========================================================================

  🛑 Stop  |  📱 QR Code  |  🔄 Refresh  |  ↩️ Back: 
```

#### Via CLI:
```sh
tailcatzero webgui
```

---

### Step 2: Forward Port on Client Machine

On your remote laptop or workstation (macOS, Linux, or Windows with `tailcat` installed):

```text
┌──(admin@laptop)-[~]
└─$ tailcat forward tcpGFwWCBVZX0y0H46ZJ_qK... 8443
[+] Connecting to WireGuard peer via DERP relay (fra)...
[+] Direct WireGuard connection established (UDP 192.168.50.1:51820)
[+] Local listener bound: 127.0.0.1:8443
[+] Forwarding TCP traffic -> router loopback 127.0.0.1:8443
[✓] Tunnel active! Open your browser to: https://localhost:8443
    (Press Ctrl+C to terminate session)
```

---

### Step 3: Open in Web Browser

Open your favorite browser and visit:
```text
https://localhost:8443
```

1. **Bypass Certificate Warning:** Since Asuswrt uses a self-signed SSL certificate, your browser will display an "Untrusted Connection" or "Your connection is not private" notice. Click **Advanced ➔ Proceed to localhost (unsafe)**.
2. **Log In:** Log into the Asuswrt-Merlin interface with your regular admin credentials.
3. You now have full, fast, responsive access to the router dashboard, wireless settings, VPN director, AiMesh nodes, and system logs!

---

## 🛑 Ending the Session

When you finish managing the router:
1. Press `Ctrl+C` on your laptop in the `tailcat forward` terminal.
2. Stop the tunnel on the router:
   * Via TUI: Press `v` ➔ Press `s`.
   * Via CLI:
     ```sh
     tailcatzero stop WEBGUI
     ```
   The token is instantly revoked, and all remote access is terminated.
