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

## 🧠 3. Volatile State & Clean Reboot Teardown

Router security tools must not leave accidental backdoors open if power is lost or a process crashes. TAILCAT ZER0 strictly enforces volatile memory storage:

* **Session Locks & PIDs:** Stored exclusively in volatile memory (`/tmp/tailcat-*` and `/tmp/tailcatzero-*`), which is backed by router RAM (`tmpfs`).
* **Clean Boot Guarantee:** Nothing is written to `/jffs/scripts/services-start` or persistent startup scripts unless explicitly automated by the user. If the router reboots, all tunnels are terminated and all state is automatically wiped clean.

---

## ⏱️ 4. The Ephemeral Watchdog

Every session launched by TAILCAT ZER0 is assigned a dedicated background watchdog process.

1. At startup, the watchdog records the creation timestamp and calculated expiration time.
2. The watchdog decrements the session lifetime continuously.
3. Upon reaching zero, the watchdog sends a `SIGTERM` signal to the service process, waits 3 seconds, and follows up with `SIGKILL` if the process has not exited.
4. Volatile PID and token files are unlinked, rendering the capability token instantly dead.

---

## 🛡️ 5. Threat Model Analysis

| Threat Scenario | Potential Impact | TAILCAT ZER0 Defense |
|---|---|---|
| **Token Interception / Leaked Token** | Unauthorized connection to running session | *Tokens are ephemeral (auto-kill in 30m) and can be revoked instantly via `tailcatzero stop` or pressing `s` in TUI.* |
| **Malicious Guest in Root Shell** | Full router compromise | *User is warned: Root shell should only be shared with 100% trusted individuals. Use View-Only Shell for third parties.* |
| **Sandbox Breakout in View Shell** | Privilege escalation to root | *Multi-layer parser blocks redirections (`>`), subshells (`$()`), chaining (`;`), sensitive files (`/etc/shadow`), and GTFOBin vectors (`sort -o`, `env`).* |
| **Hardware Flash Corruption** | Permanent router bricking | *Hard Red Lines strictly prohibit `dd of=/dev/mtd*`, `flash_erase*`, `rm -rf /`, and `nvram erase` from ever running.* |
| **MITM on Relay Network** | Eavesdropping on session traffic | *Traffic is protected with Noise/WireGuard end-to-end encryption. DERP relays have zero visibility into plaintext data.* |
| **Credential Harvesting** | Storing passwords or stealing keys | *`nvram show` automatically scrubs credentials; `/etc/shadow` and SSH private keys are blocked from read commands.* |
