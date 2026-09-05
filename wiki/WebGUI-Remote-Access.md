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
3. Choose the target port:
   * **Option 1:** HTTPS Port `8443` (Default).
   * **Option 2:** HTTP Port `80`.

#### Via CLI:
```sh
tailcatzero webgui
```

TAILCAT ZER0 generates a unique capability token (e.g. `tc9a4f21de805c31`).

---

### Step 2: Forward Port on Client Machine

On your remote laptop or workstation (macOS, Linux, or Windows with `tailcat` installed):

```sh
tailcat forward tc9a4f21de805c31 8443
```
*Expected output:*
```text
Forwarding localhost:8443 -> tc9a4f21de805c31:8443...
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
