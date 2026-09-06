/* =========================================================================================================================
 * TAILCAT ZER0 — Hardened View-Only Diagnostic Shell in C99 (Musl / Static)
 * https://github.com/underd0se/tailcat-zero
 * ========================================================================================================================= */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <glob.h>
#include <limits.h>
#include <syslog.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* IPC Paths */
#define SESSIONS_DIR    "/tmp/tailcat_sessions"
#define REQUESTS_DIR    "/tmp/tailcat_sessions/requests"
#define APPROVED_FILE   "/tmp/tailcat_sessions/VIEW_APPROVED.txt"
#define DENIED_FILE     "/tmp/tailcat_sessions/VIEW_DENIED.txt"
#define VIEW_ONCE_DIR   "/tmp/tailcat_sessions/VIEW_ONCE"
#define ACTIVE_TUIS_DIR "/tmp/tailcat_sessions/active_tuis"

/* ANSI Colors */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_GREEN   "\033[1;32m"
#define C_CYAN    "\033[1;36m"
#define C_YELLOW  "\033[1;33m"
#define C_RED     "\033[1;31m"
#define C_GRAY    "\033[0;37m"

#define MAX_LINE_LEN   4096
#define MAX_STAGES     32
#define MAX_ARGS       128

extern char **environ;

/* Forward declarations */
static int validate_and_run(const char *raw_input);
static int request_command_approval(const char *raw_cmd, const char *base_cmd);

/* -------------------------------------------------------------------------------------------------------------------------
 * Utility & Canonicalization Helpers
 * ------------------------------------------------------------------------------------------------------------------------- */

static void trim_whitespace(char *str) {
    if (!str) return;
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

/* Canonicalizes string for security evaluation:
 * Strips quotes (' and "), backslashes (\), normalizes tabs/spaces to single space, trims. */
static void canonicalize_str(const char *src, char *dst, size_t dst_sz) {
    if (!dst || dst_sz == 0) return;
    dst[0] = '\0';
    if (!src) return;

    size_t d = 0;
    int prev_space = 0;

    while (*src && isspace((unsigned char)*src)) src++;

    while (*src && d + 1 < dst_sz) {
        char c = *src++;
        if (c == '\\' || c == '\'' || c == '"') {
            continue;
        }
        if (isspace((unsigned char)c)) {
            if (!prev_space && d > 0 && d + 1 < dst_sz) {
                dst[d++] = ' ';
                prev_space = 1;
            }
        } else {
            dst[d++] = c;
            prev_space = 0;
        }
    }

    if (d > 0 && dst[d - 1] == ' ') {
        d--;
    }
    dst[d] = '\0';
}

static const char *get_base_name(const char *path) {
    if (!path) return "";
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

/* -------------------------------------------------------------------------------------------------------------------------
 * IPC & Approval State
 * ------------------------------------------------------------------------------------------------------------------------- */

static int is_cmd_approved(const char *cmd) {
    if (!cmd || !*cmd) return 0;

    /* 1. Session-wide approval file */
    FILE *fp = fopen(APPROVED_FILE, "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = '\0';
            if (strcmp(line, cmd) == 0) {
                fclose(fp);
                return 1;
            }
        }
        fclose(fp);
    }

    /* 2. Single-use token file */
    char token_path[PATH_MAX];
    snprintf(token_path, sizeof(token_path), "%s/%s.token", VIEW_ONCE_DIR, cmd);
    if (access(token_path, F_OK) == 0) {
        unlink(token_path);
        return 1;
    }

    return 0;
}

static int is_cmd_denied(const char *cmd) {
    if (!cmd || !*cmd) return 0;
    FILE *fp = fopen(DENIED_FILE, "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = '\0';
            if (strcmp(line, cmd) == 0) {
                fclose(fp);
                return 1;
            }
        }
        fclose(fp);
    }
    return 0;
}

static int is_gtfobin(const char *cmd) {
    static const char *const gtfo[] = {
        "awk", "gawk", "mawk", "nawk", "find", "python", "python2", "python3",
        "perl", "lua", "ruby", "php", "vim", "vi", "nano", "ed", "sed",
        "tar", "zip", "unzip", "gzip", "bzip2", "cp", "mv", "rm", "dd",
        "chmod", "chown", "sh", "bash", "ash", "zsh", "dash", "ksh",
        "busybox", "env", "xargs", "tee", "curl", "wget", "nc", "netcat", "socat",
        NULL
    };
    for (int i = 0; gtfo[i]; i++) {
        if (strcmp(cmd, gtfo[i]) == 0) return 1;
    }
    return 0;
}

/* -------------------------------------------------------------------------------------------------------------------------
 * Security Filter Checks
 * ------------------------------------------------------------------------------------------------------------------------- */

static const char *case_str_search(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    size_t needle_len = strlen(needle);
    if (needle_len == 0) return haystack;
    for (; *haystack; haystack++) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
            size_t j = 1;
            for (; j < needle_len; j++) {
                if (tolower((unsigned char)haystack[j]) != tolower((unsigned char)needle[j])) {
                    break;
                }
            }
            if (j == needle_len) return haystack;
        }
    }
    return NULL;
}

static int check_proc_component(const char *path, const char *comp) {
    size_t clen = strlen(comp);
    const char *ptr = path;
    while ((ptr = case_str_search(ptr, comp)) != NULL) {
        if (ptr > path && *(ptr - 1) == '/') {
            char next = ptr[clen];
            if (next == '\0' || next == '/' || next == ' ' || next == '\t' ||
                next == '"' || next == '\'' || next == '|') {
                return 1;
            }
        }
        ptr += clen;
    }
    return 0;
}

static int check_sensitive_target(const char *p) {
    if (!p || !*p) return 0;

    /* /dev/null is explicitly permitted */
    if (strcmp(p, "/dev/null") == 0) return 0;

    /* Check /dev/ paths: if any /dev/ is not /dev/null, it is blocked */
    const char *dev_match = strstr(p, "/dev/");
    while (dev_match) {
        if (strncmp(dev_match, "/dev/null", 9) == 0) {
            char next = dev_match[9];
            if (next == '\0' || next == ' ' || next == '\t' || next == '|' ||
                next == '"' || next == '\'' || next == '/') {
                dev_match = strstr(dev_match + 9, "/dev/");
                continue;
            }
        }
        return 1;
    }

    /* Check sensitive /proc/ targets: /proc/<pid>/environ, /proc/<pid>/mem, /proc/<pid>/fd, /proc/kcore, etc. */
    if (case_str_search(p, "/proc/") != NULL || strncmp(p, "/proc", 5) == 0) {
        static const char *const proc_blocked[] = {
            "environ", "mem", "kcore", "kmsg", "kallsyms", "keys", "key-users",
            "cmdline", "fd", "cwd", "root", "sysrq-trigger", NULL
        };
        for (int i = 0; proc_blocked[i]; i++) {
            if (check_proc_component(p, proc_blocked[i])) {
                return 1;
            }
        }
    }

    static const char *const sens_patterns[] = {
        "shadow", "passwd", "etc/sha", "etc/pas", ".ssh", "dropbear",
        "/openvpn", "openvpn/", ".key", "/jffs/ssl/", "/jffs/.sys",
        "tailcat_sessions", "tailcat_addr", "authorized_keys", "id_",
        ".crt", ".pem", ".pfx", ".p12", ".der", "/nvram", "nvram.nvm",
        "tailcatzero.cfg", "tailcat.cfg", "/addons/tailcat",
        "wireguard", "/wg", ".ovpn", ".pcap", ".masterkey", NULL
    };

    for (int i = 0; sens_patterns[i]; i++) {
        if (case_str_search(p, sens_patterns[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

static int check_file_path_security(const char *path) {
    if (!path || !*path) return 0;
    if (check_sensitive_target(path)) return 1;

    /* Wildcard glob check (*, ?, [) */
    if (strpbrk(path, "*?[") != NULL) {
        glob_t gb;
        if (glob(path, 0, NULL, &gb) == 0) {
            for (size_t g = 0; g < gb.gl_pathc; g++) {
                const char *match = gb.gl_pathv[g];
                if (check_sensitive_target(match)) {
                    globfree(&gb);
                    return 1;
                }
                char resolved[PATH_MAX];
                if (realpath(match, resolved) != NULL) {
                    if (check_sensitive_target(resolved)) {
                        globfree(&gb);
                        return 1;
                    }
                }
            }
            globfree(&gb);
        }
    }

    /* Direct symlink & canonical target check */
    struct stat st;
    if (lstat(path, &st) == 0) {
        if (S_ISLNK(st.st_mode)) {
            char link_target[PATH_MAX];
            ssize_t len = readlink(path, link_target, sizeof(link_target) - 1);
            if (len > 0) {
                link_target[len] = '\0';
                if (check_sensitive_target(link_target)) return 1;
            }
        }
        char resolved[PATH_MAX];
        if (realpath(path, resolved) != NULL) {
            if (check_sensitive_target(resolved)) return 1;
        }
    }
    return 0;
}

static int is_sensitive_file_access(const char *canon_str, char *const argv[], int argc) {
    if (check_sensitive_target(canon_str)) return 1;

    for (int i = 0; i < argc; i++) {
        const char *tok = argv[i];
        if (!tok || !*tok) continue;

        if (check_file_path_security(tok)) return 1;

        /* If token is an option containing an embedded file path, e.g. --files0-from=/tmp/sym, --file=/tmp/sym, -f/tmp/sym */
        if (tok[0] == '-') {
            const char *eq = strchr(tok, '=');
            if (eq && *(eq + 1)) {
                if (check_file_path_security(eq + 1)) return 1;
            }
            if (tok[1] == 'f' && tok[2] != '\0') {
                if (check_file_path_security(tok + 2)) return 1;
            }
        }
    }
    return 0;
}

static int is_sensitive_file_access_str(const char *canon_str) {
    if (!canon_str || !*canon_str) return 0;
    char buf[MAX_LINE_LEN];
    strncpy(buf, canon_str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *toks[MAX_ARGS];
    int ntoks = 0;
    char *saveptr = NULL;
    char *p = strtok_r(buf, " \t", &saveptr);
    while (p && ntoks < MAX_ARGS - 1) {
        toks[ntoks++] = p;
        p = strtok_r(NULL, " \t", &saveptr);
    }
    toks[ntoks] = NULL;
    return is_sensitive_file_access(canon_str, toks, ntoks);
}

static int is_sensitive_nvram_key(const char *key) {
    if (!key || !*key) return 0;
    if (key[0] == '-') return 0;

    static const char *const nv_patterns[] = {
        "pass", "cert", "crt", "priv", "secret", "key",
        "psk", "pin", "token", "auth", "user", "acc_",
        "hash", "salt", "cred", "seed", "ovpn", "wg",
        "cookie", "session", "otp", "totp", "api",
        NULL
    };

    for (int i = 0; nv_patterns[i]; i++) {
        if (case_str_search(key, nv_patterns[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------------------------------------------------------
 * Built-in Command Displays
 * ------------------------------------------------------------------------------------------------------------------------- */

static void print_banner(void) {
    printf("%s%s========================================================================%s\n", C_CYAN, C_BOLD, C_RESET);
    printf("  %s%s🐱 TAILCAT ZER0 — View-Only Diagnostic Shell%s\n", C_CYAN, C_BOLD, C_RESET);
    printf("  WireGuard P2P Encrypted Session • %sRead-Only Diagnostics%s\n", C_GREEN, C_RESET);
    printf("  (All system writes, file modifications & process kills are blocked)\n");
    printf("%s========================================================================%s\n\n", C_CYAN, C_RESET);
    printf("  Type %shelp%s for tools, %ssysinfo%s for status, %srequest <cmd>%s for approval.\n\n",
           C_YELLOW, C_RESET, C_YELLOW, C_RESET, C_YELLOW, C_RESET);
}

static void show_help(void) {
    printf("  %sAvailable Read-Only Diagnostic Commands:%s\n\n", C_BOLD, C_RESET);

    printf("  %s📊 System Health & Hardware:%s\n", C_CYAN, C_RESET);
    printf("    uptime, date, free, df, ps, top, uname, dmesg, lsmod\n");
    printf("    sysinfo, cpuinfo, meminfo, temperature\n\n");

    printf("  %s🌐 Network, WiFi & Routing:%s\n", C_CYAN, C_RESET);
    printf("    ip [addr|route|neigh|link|rule], ifconfig, netstat, route, arp\n");
    printf("    ping, ping6, traceroute, traceroute6, mtr, nslookup, dig, host\n");
    printf("    wl (WiFi stats & clients), ethtool, ethctl, robocfg\n");
    printf("    leases (DHCP clients), wifi, ports\n\n");

    printf("  %s📦 Entware Package Inspection (opkg):%s\n", C_CYAN, C_RESET);
    printf("    opkg list, opkg list-installed, opkg info <pkg>\n");
    printf("    opkg find <query>, opkg status [pkg], opkg search <file>\n");
    printf("    opkg depends <pkg>, opkg whatdepends <pkg>\n");
    printf("    %s(Note: opkg install/remove/upgrade are blocked)%s\n\n", C_GRAY, C_RESET);

    printf("  %s📄 Router Config & Logs:%s\n", C_CYAN, C_RESET);
    printf("    nvram get <key> (credential keys blocked), logread, logread -f, logs\n\n");

    printf("  %s🔍 Text & File Inspection:%s\n", C_CYAN, C_RESET);
    printf("    cat, head, tail (tail -f), more, less, grep, egrep, fgrep, rg\n");
    printf("    tree, ls, dir, du, wc, sort, uniq, cut, column, tr, diff\n");
    printf("    strings, stat, file, hexdump, locate, which, whereis\n\n");

    printf("  %s⚡ Pipelines:%s\n", C_CYAN, C_RESET);
    printf("    Pipes (|) between allowed commands are supported (e.g. ps | grep tailcat)\n");
    printf("    File redirections (>, >>, <) and command chaining (; && ||) are blocked.\n\n");

    printf("  %s🔔 On-Demand Permission Escalation:%s\n", C_CYAN, C_RESET);
    printf("    request <cmd> (e.g. request tcpdump -i eth0)\n");
    printf("    Submits a real-time request to the router host for approval.\n");
    printf("    Host can approve for the entire session or for a single run.\n\n");
}

static void run_nvram_get_safe(const char *key, char *buf, size_t sz, const char *dflt) {
    buf[0] = '\0';
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "nvram get %s 2>/dev/null", key);
    FILE *p = popen(cmd, "r");
    if (p) {
        if (fgets(buf, sz, p)) {
            buf[strcspn(buf, "\r\n")] = '\0';
        }
        pclose(p);
    }
    if (!buf[0] && dflt) {
        strncpy(buf, dflt, sz - 1);
        buf[sz - 1] = '\0';
    }
}

static void show_sysinfo(void) {
    char model[128], buildno[64], extendno[64], wan_ip[64], wan_proto[64], lan_ip[64];
    run_nvram_get_safe("model", model, sizeof(model), "Unknown Router");
    run_nvram_get_safe("buildno", buildno, sizeof(buildno), "Unknown");
    run_nvram_get_safe("extendno", extendno, sizeof(extendno), "");
    run_nvram_get_safe("wan0_ipaddr", wan_ip, sizeof(wan_ip), "N/A");
    run_nvram_get_safe("wan0_proto", wan_proto, sizeof(wan_proto), "N/A");
    run_nvram_get_safe("lan_ipaddr", lan_ip, sizeof(lan_ip), "N/A");

    printf("\n%s%s--- Router System Summary ---%s\n", C_CYAN, C_BOLD, C_RESET);
    printf("  %sModel:%s        %s (Firmware: %s_%s)\n", C_BOLD, C_RESET, model, buildno, extendno);

    char uptime_str[256] = "N/A";
    FILE *up = popen("uptime 2>/dev/null", "r");
    if (up) {
        if (fgets(uptime_str, sizeof(uptime_str), up)) {
            uptime_str[strcspn(uptime_str, "\r\n")] = '\0';
            char *p = uptime_str;
            while (*p && isspace((unsigned char)*p)) p++;
            printf("  %sUptime:%s       %s\n", C_BOLD, C_RESET, p);
        }
        pclose(up);
    }

    printf("  %sWAN IP:%s       %s (Protocol: %s)\n", C_BOLD, C_RESET, wan_ip, wan_proto);
    printf("  %sLAN IP:%s       %s\n", C_BOLD, C_RESET, lan_ip);

    FILE *tf = fopen("/proc/dmu/temperature", "r");
    if (tf) {
        char tval[64];
        if (fgets(tval, sizeof(tval), tf)) {
            tval[strcspn(tval, "\r\n")] = '\0';
            if (tval[0]) {
                printf("  %sTemperature:%s %s\n", C_BOLD, C_RESET, tval);
            }
        }
        fclose(tf);
    }

    printf("  %sMemory:%s\n", C_BOLD, C_RESET);
    fflush(stdout);
    if (system("free -h 2>/dev/null || free -m 2>/dev/null || free 2>/dev/null") != 0) {
        /* fallback ignore */
    }

    printf("\n  %sStorage:%s\n", C_BOLD, C_RESET);
    fflush(stdout);
    if (system("df -h /jffs /tmp 2>/dev/null || df -h 2>/dev/null") != 0) {
        /* fallback ignore */
    }

    printf("%s----------------------------%s\n\n", C_CYAN, C_RESET);
}

static void show_leases(void) {
    printf("\n%sActive DHCP Leases (/tmp/dnsmasq.leases):%s\n\n", C_BOLD, C_RESET);
    FILE *fp = fopen("/tmp/dnsmasq.leases", "r");
    if (fp) {
        printf("%s%-16s %-18s %-25s %s%s\n", C_CYAN, "IP Address", "MAC Address", "Hostname", "Client ID", C_RESET);
        printf("%s\n", "────────────────────────────────────────────────────────────────────────");
        char line[512];
        int count = 0;
        while (fgets(line, sizeof(line), fp)) {
            char tstamp[64], mac[64], ip[64], host[128], clid[128];
            tstamp[0] = mac[0] = ip[0] = host[0] = clid[0] = '\0';
            if (sscanf(line, "%63s %63s %63s %127s %127s", tstamp, mac, ip, host, clid) >= 4) {
                if (strcmp(host, "*") == 0) strcpy(host, "<unnamed>");
                printf("%-16s %-18s %-25s %s\n", ip, mac, host, clid);
                count++;
            }
        }
        fclose(fp);
        if (count == 0) {
            printf("  No active DHCP leases found.\n\n");
        } else {
            printf("\n");
        }
    } else {
        printf("  No active DHCP leases found.\n\n");
    }
}

static void show_wifi(void) {
    printf("\n%sWireless Radio & Connected Clients:%s\n\n", C_BOLD, C_RESET);
    const char *ifaces[] = { "eth6", "eth7", "wl0", "wl1", NULL };
    for (int i = 0; ifaces[i]; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "command -v wl >/dev/null 2>&1 && wl -i %s isup 2>/dev/null", ifaces[i]);
        FILE *p = popen(cmd, "r");
        if (p) {
            char buf[32] = {0};
            if (fgets(buf, sizeof(buf), p) && strstr(buf, "1")) {
                pclose(p);
                char ssid[128] = "N/A", chanspec[64] = "N/A", client_cnt[32] = "0";

                snprintf(cmd, sizeof(cmd), "wl -i %s ssid 2>/dev/null | sed 's/^Current SSID: //'", ifaces[i]);
                FILE *fs = popen(cmd, "r");
                if (fs) {
                    if (fgets(ssid, sizeof(ssid), fs)) ssid[strcspn(ssid, "\r\n")] = '\0';
                    pclose(fs);
                }

                snprintf(cmd, sizeof(cmd), "wl -i %s chanspec 2>/dev/null", ifaces[i]);
                FILE *fc = popen(cmd, "r");
                if (fc) {
                    if (fgets(chanspec, sizeof(chanspec), fc)) chanspec[strcspn(chanspec, "\r\n")] = '\0';
                    pclose(fc);
                }

                snprintf(cmd, sizeof(cmd), "wl -i %s assoclist 2>/dev/null | wc -l | tr -d ' '", ifaces[i]);
                FILE *fa = popen(cmd, "r");
                if (fa) {
                    if (fgets(client_cnt, sizeof(client_cnt), fa)) client_cnt[strcspn(client_cnt, "\r\n")] = '\0';
                    pclose(fa);
                }

                printf("  • %sInterface %s:%s SSID: %s%s%s (Channel: %s) • Clients: %s%s%s\n",
                       C_BOLD, ifaces[i], C_RESET, C_CYAN, ssid, C_RESET, chanspec, C_GREEN, client_cnt, C_RESET);
                continue;
            }
            pclose(p);
        }
    }
    printf("\n");
}

static void show_ports(void) {
    printf("\n%sActive Listening Services (TCP & UDP):%s\n\n", C_BOLD, C_RESET);
    fflush(stdout);
    if (system("netstat -tlpn 2>/dev/null || netstat -an 2>/dev/null") != 0) {
        /* ignore */
    }
    printf("\n");
}

/* -------------------------------------------------------------------------------------------------------------------------
 * Permission Escalation Engine
 * ------------------------------------------------------------------------------------------------------------------------- */

static void prompt_or_hint_approval(const char *cmd_to_req, const char *base_cmd) {
    if (isatty(STDIN_FILENO)) {
        printf("    Would you like to request host approval for '%s'? [y/N]: ", cmd_to_req);
        fflush(stdout);
        char resp[32];
        if (fgets(resp, sizeof(resp), stdin)) {
            if (resp[0] == 'y' || resp[0] == 'Y') {
                request_command_approval(cmd_to_req, base_cmd);
            }
        }
    } else {
        printf("    Type 'request %s' to ask host for approval.\n", cmd_to_req);
    }
}

static int request_command_approval(const char *raw_cmd, const char *base_cmd) {
    char canon[MAX_LINE_LEN];
    canonicalize_str(raw_cmd, canon, sizeof(canon));

    /* Red lines: permanent hardware and partition protection */
    if (strstr(canon, "/dev/mtd") != NULL ||
        strstr(canon, "/dev/nvram") != NULL ||
        strstr(canon, "/dev/mem") != NULL ||
        strstr(canon, "/dev/kmem") != NULL ||
        strstr(canon, "rm -rf /") != NULL ||
        strstr(canon, "nvram erase") != NULL ||
        strstr(canon, "mtd ") != NULL ||
        strstr(canon, "mtd-") != NULL ||
        strstr(canon, "flash_erase") != NULL ||
        strstr(canon, "flashcp") != NULL ||
        strstr(canon, "nandwrite") != NULL) {
        fprintf(stderr, "%s[!] Security Error: Destructive hardware / partition operations cannot be requested.%s\n", C_RED, C_RESET);
        return 1;
    }

    /* Sensitive files check */
    if (is_sensitive_file_access_str(canon)) {
        fprintf(stderr, "%s[!] Security Error: Requests targeting sensitive system security files are prohibited in view-only mode.%s\n", C_RED, C_RESET);
        return 1;
    }

    mkdir(SESSIONS_DIR, 0700);
    mkdir(REQUESTS_DIR, 0700);
    mkdir(VIEW_ONCE_DIR, 0700);

    time_t now = time(NULL);
    pid_t pid = getpid();
    char req_id[128];
    snprintf(req_id, sizeof(req_id), "req_%ld_%d", (long)now, (int)pid);

    char req_file[PATH_MAX], resp_file[PATH_MAX];
    snprintf(req_file, sizeof(req_file), "%s/%s.req", REQUESTS_DIR, req_id);
    snprintf(resp_file, sizeof(resp_file), "%s/%s.resp", REQUESTS_DIR, req_id);

    /* Sanitize base_cmd */
    char base_clean[64];
    size_t b = 0;
    for (const char *p = base_cmd; *p && b + 1 < sizeof(base_clean); p++) {
        if (isalnum((unsigned char)*p) || *p == '_' || *p == '.' || *p == '-') {
            base_clean[b++] = *p;
        }
    }
    base_clean[b] = '\0';

    if (!base_clean[0] || strcmp(base_clean, ".") == 0 || strcmp(base_clean, "..") == 0) {
        fprintf(stderr, "%s[!] Security Error: Invalid or prohibited tool name in request.%s\n", C_RED, C_RESET);
        return 1;
    }

    int gtfo = is_gtfobin(base_clean);

    /* Write request file atomically with 0600 permissions */
    int rfd = open(req_file, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (rfd < 0) {
        fprintf(stderr, "%s[!] Error creating request file.%s\n", C_RED, C_RESET);
        return 1;
    }
    FILE *rf = fdopen(rfd, "w");
    if (!rf) {
        close(rfd);
        unlink(req_file);
        fprintf(stderr, "%s[!] Error opening request file stream.%s\n", C_RED, C_RESET);
        return 1;
    }
    fprintf(rf, "REQ_ID=\"%s\"\n", req_id);
    fprintf(rf, "GUEST_PID=\"%d\"\n", (int)pid);
    fprintf(rf, "BASE_CMD=\"%s\"\n", base_clean);
    fprintf(rf, "RAW_COMMAND=\"");
    for (const char *p = raw_cmd; *p; p++) {
        if (*p == '"' || *p == '\\' || *p == '$' || *p == '`') {
            fputc('\\', rf);
        }
        fputc(*p, rf);
    }
    fprintf(rf, "\"\n");
    fprintf(rf, "CREATED_AT=\"%ld\"\n", (long)now);
    fprintf(rf, "IS_GTFOBIN=\"%s\"\n", gtfo ? "YES" : "NO");
    fprintf(rf, "STATUS=\"PENDING\"\n");
    fclose(rf);

    /* Syslog */
    openlog("tailcatzero", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "GUEST PERMISSION REQUEST: '%s' ('%s') [ID: %s] GTFO=%s", base_clean, raw_cmd, req_id, gtfo ? "YES" : "NO");
    closelog();

    /* Notify active TUIs */
    char cur_tty[128] = {0};
    if (isatty(STDIN_FILENO)) {
        const char *t = ttyname(STDIN_FILENO);
        if (t) strncpy(cur_tty, t, sizeof(cur_tty) - 1);
    }

    char tty_safe[81];
    size_t s = 0;
    for (const char *p = raw_cmd; *p && s + 1 < sizeof(tty_safe); p++) {
        if (isalnum((unsigned char)*p) || *p == ' ' || *p == '_' || *p == '.' ||
            *p == '/' || *p == ':' || *p == '=' || *p == '+' || *p == '-') {
            tty_safe[s++] = *p;
        }
    }
    tty_safe[s] = '\0';

    DIR *dir = opendir(ACTIVE_TUIS_DIR);
    if (dir) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (de->d_name[0] == '.') continue;
            pid_t t_pid = (pid_t)atoi(de->d_name);
            if (t_pid > 0 && kill(t_pid, 0) == 0) {
                char fpath[PATH_MAX];
                snprintf(fpath, sizeof(fpath), "%s/%s", ACTIVE_TUIS_DIR, de->d_name);
                FILE *tf = fopen(fpath, "r");
                if (tf) {
                    char target_tty[128];
                    if (fgets(target_tty, sizeof(target_tty), tf)) {
                        target_tty[strcspn(target_tty, "\r\n")] = '\0';
                        if (target_tty[0] && strcmp(target_tty, cur_tty) != 0) {
                            int t_fd = open(target_tty, O_WRONLY | O_NOCTTY);
                            if (t_fd >= 0) {
                                char tty_msg[512];
                                if (gtfo) {
                                    snprintf(tty_msg, sizeof(tty_msg), "\n\033[1;31m[tailcatzero] ⚠️  Guest requested elevated execution tool: '%s' (ID: %s)\033[0m\n\033[1;36mPress [P] to review or run 'tailcatzero approve'\033[0m\n", tty_safe, req_id);
                                } else {
                                    snprintf(tty_msg, sizeof(tty_msg), "\n\033[1;33m[tailcatzero] 🔔 Guest requested permission to run: '%s' (ID: %s)\033[0m\n\033[1;36mPress [P] to review or run 'tailcatzero approve'\033[0m\n", tty_safe, req_id);
                                }
                                ssize_t written = write(t_fd, tty_msg, strlen(tty_msg));
                                (void)written;
                                close(t_fd);
                            }
                        }
                    }
                    fclose(tf);
                }
            } else {
                char stale_path[PATH_MAX];
                snprintf(stale_path, sizeof(stale_path), "%s/%s", ACTIVE_TUIS_DIR, de->d_name);
                unlink(stale_path);
            }
        }
        closedir(dir);
    }

    printf("\n%s[*] Permission request submitted to router host:%s\n", C_CYAN, C_RESET);
    printf("    Command: %s%s%s (Tool: %s%s%s)\n", C_YELLOW, raw_cmd, C_RESET, C_BOLD, base_clean, C_RESET);
    if (gtfo) {
        printf("    %s⚠️  Notice: '%s' has execution or file modification capabilities.%s\n", C_YELLOW, base_clean, C_RESET);
    }
    printf("    ID:      %s\n", req_id);
    printf("%s[*] Waiting for host approval (timeout: 60s)...%s ", C_GRAY, C_RESET);
    fflush(stdout);

    int waited = 0;
    char resp_status[64] = {0};
    while (waited < 60) {
        FILE *sf = fopen(resp_file, "r");
        if (sf) {
            char line[256];
            while (fgets(line, sizeof(line), sf)) {
                if (strncmp(line, "STATUS=", 7) == 0) {
                    char *val = line + 7;
                    while (*val == '"') val++;
                    val[strcspn(val, "\"\r\n")] = '\0';
                    strncpy(resp_status, val, sizeof(resp_status) - 1);
                    break;
                }
            }
            fclose(sf);
            if (resp_status[0]) break;
        }
        sleep(1);
        waited++;
        if (waited % 5 == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    printf("\n");

    unlink(req_file);
    unlink(resp_file);

    if (strcmp(resp_status, "APPROVED_SESSION") == 0) {
        printf("%s[+] Host APPROVED '%s' for this session! Running command...%s\n\n", C_GREEN, base_clean, C_RESET);
        /* Remove from denied list */
        FILE *df = fopen(DENIED_FILE, "r");
        if (df) {
            char tmp_path[PATH_MAX];
            snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", DENIED_FILE);
            FILE *tf = fopen(tmp_path, "w");
            char line[256];
            while (fgets(line, sizeof(line), df)) {
                line[strcspn(line, "\r\n")] = '\0';
                if (strcmp(line, base_clean) != 0 && tf) {
                    fprintf(tf, "%s\n", line);
                }
            }
            fclose(df);
            if (tf) {
                fclose(tf);
                rename(tmp_path, DENIED_FILE);
            }
        }
        /* Add to approved list */
        FILE *af = fopen(APPROVED_FILE, "a+");
        if (af) {
            fprintf(af, "%s\n", base_clean);
            fclose(af);
        }
        char tok_p[PATH_MAX];
        snprintf(tok_p, sizeof(tok_p), "%s/%s.token", VIEW_ONCE_DIR, base_clean);
        unlink(tok_p);
        return validate_and_run(raw_cmd);
    } else if (strcmp(resp_status, "APPROVED_ONCE") == 0) {
        printf("%s[+] Host APPROVED single execution! Running command...%s\n\n", C_GREEN, C_RESET);
        char tok_p[PATH_MAX];
        snprintf(tok_p, sizeof(tok_p), "%s/%s.token", VIEW_ONCE_DIR, base_clean);
        unlink(tok_p);
        return validate_and_run(raw_cmd);
    } else if (strcmp(resp_status, "DENIED") == 0) {
        printf("%s[-] Host DENIED permission for '%s'.%s\n", C_RED, base_clean, C_RESET);
        FILE *df = fopen(DENIED_FILE, "a+");
        if (df) {
            fprintf(df, "%s\n", base_clean);
            fclose(df);
        }
        return 1;
    } else {
        printf("%s[!] Request timed out (host did not respond within 60s).%s\n", C_RED, C_RESET);
        return 1;
    }
}

/* -------------------------------------------------------------------------------------------------------------------------
 * Tokenizer & Parser
 * ------------------------------------------------------------------------------------------------------------------------- */

/* Splits input string on pipe '|', but preserves pipes inside single or double quotes */
static int split_pipeline(const char *input, char stages[MAX_STAGES][MAX_LINE_LEN]) {
    int count = 0;
    size_t s = 0;
    int in_sq = 0;
    int in_dq = 0;
    int esc = 0;

    for (const char *p = input; *p; p++) {
        char c = *p;
        if (s + 1 >= MAX_LINE_LEN) {
            fprintf(stderr, "%s[!] Syntax Error: Command line exceeds maximum supported length.%s\n", C_RED, C_RESET);
            return -1;
        }
        if (esc) {
            stages[count][s++] = c;
            esc = 0;
            continue;
        }
        if (c == '\\' && !in_sq) {
            esc = 1;
            stages[count][s++] = c;
            continue;
        }
        if (c == '\'' && !in_dq) {
            in_sq = !in_sq;
            stages[count][s++] = c;
            continue;
        }
        if (c == '"' && !in_sq) {
            in_dq = !in_dq;
            stages[count][s++] = c;
            continue;
        }
        if (c == '|' && !in_sq && !in_dq) {
            stages[count][s] = '\0';
            trim_whitespace(stages[count]);
            if (!stages[count][0]) {
                fprintf(stderr, "%s[!] Syntax Error: Empty pipeline stage.%s\n", C_RED, C_RESET);
                return -1;
            }
            count++;
            if (count >= MAX_STAGES) {
                fprintf(stderr, "%s[!] Syntax Error: Too many pipeline stages.%s\n", C_RED, C_RESET);
                return -1;
            }
            s = 0;
            continue;
        }
        stages[count][s++] = c;
    }
    if (in_sq || in_dq || esc) {
        fprintf(stderr, "%s[!] Syntax Error: Unclosed quote or trailing escape in command.%s\n", C_RED, C_RESET);
        return -1;
    }
    stages[count][s] = '\0';
    trim_whitespace(stages[count]);
    if (!stages[count][0]) {
        fprintf(stderr, "%s[!] Syntax Error: Empty pipeline stage.%s\n", C_RED, C_RESET);
        return -1;
    }
    return count + 1;
}

/* Tokenizes a single pipeline stage into argv[], handling single quotes, double quotes, escapes */
static int tokenize_stage(const char *stage, char *argv[MAX_ARGS], char tok_buf[MAX_LINE_LEN]) {
    int argc = 0;
    size_t b = 0;
    int in_tok = 0;
    int in_sq = 0;
    int in_dq = 0;
    int esc = 0;

    for (const char *p = stage; *p; p++) {
        char c = *p;
        if (b + 1 >= MAX_LINE_LEN) {
            fprintf(stderr, "%s[!] Syntax Error: Stage arguments exceed maximum supported length.%s\n", C_RED, C_RESET);
            return 0;
        }
        if (esc) {
            if (!in_tok) {
                argv[argc++] = &tok_buf[b];
                in_tok = 1;
            }
            tok_buf[b++] = c;
            esc = 0;
            continue;
        }
        if (c == '\\' && !in_sq) {
            esc = 1;
            if (!in_tok) {
                argv[argc++] = &tok_buf[b];
                in_tok = 1;
            }
            continue;
        }
        if (c == '\'' && !in_dq) {
            in_sq = !in_sq;
            if (!in_tok) {
                argv[argc++] = &tok_buf[b];
                in_tok = 1;
            }
            continue;
        }
        if (c == '"' && !in_sq) {
            in_dq = !in_dq;
            if (!in_tok) {
                argv[argc++] = &tok_buf[b];
                in_tok = 1;
            }
            continue;
        }
        if (isspace((unsigned char)c) && !in_sq && !in_dq) {
            if (in_tok) {
                tok_buf[b++] = '\0';
                in_tok = 0;
                if (argc >= MAX_ARGS - 1) break;
            }
            continue;
        }
        if (!in_tok) {
            argv[argc++] = &tok_buf[b];
            in_tok = 1;
        }
        tok_buf[b++] = c;
    }
    if (in_sq || in_dq || esc) {
        fprintf(stderr, "%s[!] Syntax Error: Unclosed quote or trailing escape in command stage.%s\n", C_RED, C_RESET);
        return 0;
    }
    if (in_tok) {
        tok_buf[b++] = '\0';
    }
    argv[argc] = NULL;
    return argc;
}

/* -------------------------------------------------------------------------------------------------------------------------
 * Command Validation & Execution
 * ------------------------------------------------------------------------------------------------------------------------- */

static int is_builtin(const char *base) {
    return (strcmp(base, "help") == 0 || strcmp(base, "h") == 0 || strcmp(base, "?") == 0 ||
            strcmp(base, "sysinfo") == 0 || strcmp(base, "leases") == 0 || strcmp(base, "dhcp-leases") == 0 ||
            strcmp(base, "wifi") == 0 || strcmp(base, "ports") == 0 || strcmp(base, "logs") == 0 ||
            strcmp(base, "clear") == 0 || strcmp(base, "exit") == 0 || strcmp(base, "quit") == 0 ||
            strcmp(base, "q") == 0 || strcmp(base, "env") == 0 || strcmp(base, "printenv") == 0 ||
            strcmp(base, "pwd") == 0 || strcmp(base, "cd") == 0);
}

static int run_builtin(const char *base, int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    if (strcmp(base, "help") == 0 || strcmp(base, "h") == 0 || strcmp(base, "?") == 0) {
        show_help();
        return 0;
    }
    if (strcmp(base, "pwd") == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd))) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        return 0;
    }
    if (strcmp(base, "cd") == 0) {
        const char *target_dir = (argc > 1) ? argv[1] : NULL;
        if (!target_dir || !*target_dir) {
            target_dir = getenv("HOME");
            if (!target_dir || !*target_dir) target_dir = "/tmp";
        }
        if (check_file_path_security(target_dir)) {
            fprintf(stderr, "%s[!] Security Error: Access to sensitive system security directories is prohibited in view-only mode.%s\n", C_RED, C_RESET);
            return 1;
        }
        if (chdir(target_dir) != 0) {
            fprintf(stderr, "cd: %s: %s\n", target_dir, strerror(errno));
            return 1;
        }
        return 0;
    }
    if (strcmp(base, "sysinfo") == 0) {
        show_sysinfo();
        return 0;
    }
    if (strcmp(base, "leases") == 0 || strcmp(base, "dhcp-leases") == 0) {
        show_leases();
        return 0;
    }
    if (strcmp(base, "wifi") == 0) {
        show_wifi();
        return 0;
    }
    if (strcmp(base, "ports") == 0) {
        show_ports();
        return 0;
    }
    if (strcmp(base, "logs") == 0) {
        if (system("logread 2>/dev/null | tail -n 50") != 0) {
            /* ignore */
        }
        return 0;
    }
    if (strcmp(base, "clear") == 0) {
        printf("\033[2J\033[H");
        fflush(stdout);
        return 0;
    }
    if (strcmp(base, "exit") == 0 || strcmp(base, "quit") == 0 || strcmp(base, "q") == 0) {
        printf("\n%sSession disconnected.%s\n", C_CYAN, C_RESET);
        exit(0);
    }
    if (strcmp(base, "env") == 0 || strcmp(base, "printenv") == 0) {
        for (char **env = environ; *env; env++) {
            printf("%s\n", *env);
        }
        return 0;
    }
    return 1;
}

static int validate_stage(const char *stage_raw, const char *orig_input,
                          char *argv[MAX_ARGS], int *p_argc, char tok_buf[MAX_LINE_LEN],
                          int *p_is_safe_stream, int *p_is_top_batch) {
    char stage_canon[MAX_LINE_LEN];
    canonicalize_str(stage_raw, stage_canon, sizeof(stage_canon));

    if (!stage_canon[0]) {
        fprintf(stderr, "%s[!] Syntax Error: Empty pipeline stage after canonicalization.%s\n", C_RED, C_RESET);
        return 1;
    }

    int argc = tokenize_stage(stage_raw, argv, tok_buf);
    *p_argc = argc;
    if (argc == 0) return 1;

    const char *base_cmd = get_base_name(argv[0]);
    const char *sub_arg = (argc > 1) ? argv[1] : NULL;

    /* Base command allowlist check */
    static const char *const allowed[] = {
        "uptime", "free", "df", "ps", "top", "uname", "dmesg", "sysinfo",
        "cpuinfo", "meminfo", "temperature", "lsmod", "netstat", "traceroute",
        "traceroute6", "mtr", "nslookup", "dig", "host", "leases", "dhcp-leases",
        "wifi", "ports", "logs", "help", "exit", "quit", "clear", "cut",
        "column", "tr", "locate", "which", "whereis", "echo", "printf",
        "who", "w", "id", "ping", "ping6", "date", "robocfg", "ethtool",
        "mii-tool", "ethctl", "cat", "head", "tail", "more", "less",
        "grep", "egrep", "fgrep", "rg", "tree", "ls", "dir", "vdir",
        "du", "wc", "sort", "uniq", "diff", "strings", "hexdump", "stat",
        "file", "route", "arp", "wl", "nvram", "opkg", "ip", "ifconfig",
        "logread", "env", "printenv", "pwd", "cd", NULL
    };

    int in_allowed = 0;
    for (int i = 0; allowed[i]; i++) {
        if (strcmp(base_cmd, allowed[i]) == 0) {
            in_allowed = 1;
            break;
        }
    }

    if (!in_allowed && !is_cmd_approved(base_cmd)) {
        if (is_cmd_denied(base_cmd)) {
            fprintf(stderr, "%s[!] Security Error: Command '%s' has been denied by host for this session.%s\n", C_RED, base_cmd, C_RESET);
            return 1;
        }
        fprintf(stderr, "%s[!] Security Error: Command '%s' is not permitted in view-only mode.%s\n", C_RED, base_cmd, C_RESET);
        prompt_or_hint_approval(orig_input, base_cmd);
        return 1;
    }

    /* Specific tool restrictions */
    if (strcmp(base_cmd, "env") == 0 || strcmp(base_cmd, "printenv") == 0) {
        if (argc > 1) {
            fprintf(stderr, "%s[!] Security Error: Executing commands via 'env' is prohibited in view-only mode. Type 'env' alone to view environment variables.%s\n", C_RED, C_RESET);
            return 1;
        }
    } else if (strcmp(base_cmd, "dmesg") == 0) {
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
                if (strchr(argv[i], 'c') || strchr(argv[i], 'C')) {
                    fprintf(stderr, "%s[!] Security Error: Clearing kernel ring buffer (dmesg -c/-C) is prohibited in view-only mode.%s\n", C_RED, C_RESET);
                    return 1;
                }
            }
        }
    } else if (strcmp(base_cmd, "ping") == 0 || strcmp(base_cmd, "ping6") == 0) {
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
                if (strchr(argv[i], 'f') || strchr(argv[i], 'F')) {
                    fprintf(stderr, "%s[!] Security Error: Flood pinging (ping -f) is prohibited in view-only mode.%s\n", C_RED, C_RESET);
                    return 1;
                }
            }
        }
    } else if (strcmp(base_cmd, "date") == 0) {
        if (is_sensitive_file_access(stage_canon, argv, argc)) {
            fprintf(stderr, "%s[!] Security Error: Access to sensitive system security files is prohibited in view-only mode.%s\n", C_RED, C_RESET);
            return 1;
        }
        for (int i = 1; i < argc; i++) {
            const char *a = argv[i];
            if (strcmp(a, "-s") == 0 || strcmp(a, "-S") == 0 || strncmp(a, "--set", 5) == 0 ||
                (a[0] == '-' && (strchr(a, 's') || strchr(a, 'S')))) {
                fprintf(stderr, "%s[!] Security Error: Modifying system date/time is prohibited in view-only mode.%s\n", C_RED, C_RESET);
                return 1;
            }
            if (isdigit((unsigned char)a[0]) && isdigit((unsigned char)a[1]) &&
                isdigit((unsigned char)a[2]) && isdigit((unsigned char)a[3])) {
                fprintf(stderr, "%s[!] Security Error: Modifying system date/time is prohibited in view-only mode.%s\n", C_RED, C_RESET);
                return 1;
            }
        }
    } else if (strcmp(base_cmd, "robocfg") == 0) {
        int mut = 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "show") != 0) {
                mut = 1;
                break;
            }
        }
        if (mut && !is_cmd_approved("robocfg")) {
            fprintf(stderr, "%s[!] Security Error: Hardware switch mutation with robocfg is prohibited in view-only mode. Only 'robocfg show' is permitted.%s\n", C_RED, C_RESET);
            return 1;
        }
    } else if (strcmp(base_cmd, "ethtool") == 0) {
        int mut = 0;
        for (int i = 1; i < argc; i++) {
            const char *a = argv[i];
            if (a[0] == '-') {
                if (strpbrk(a, "sKkACGpPrReEd") != NULL ||
                    strcmp(a, "--change") == 0 || strcmp(a, "--features") == 0 ||
                    strcmp(a, "--pause") == 0 || strcmp(a, "--coalesce") == 0 ||
                    strcmp(a, "--ring") == 0 || strcmp(a, "--identify") == 0 ||
                    strcmp(a, "--reset") == 0) {
                    mut = 1;
                    break;
                }
            }
        }
        if (mut && !is_cmd_approved("ethtool")) {
            fprintf(stderr, "%s[!] Security Error: Hardware interface mutation with ethtool is prohibited in view-only mode.%s\n", C_RED, C_RESET);
            return 1;
        }
    } else if (strcmp(base_cmd, "mii-tool") == 0) {
        int mut = 0;
        for (int i = 1; i < argc; i++) {
            const char *a = argv[i];
            if (a[0] == '-') {
                if (strpbrk(a, "FfrR") != NULL ||
                    strcmp(a, "--force") == 0 || strcmp(a, "--restart") == 0) {
                    mut = 1;
                    break;
                }
            }
        }
        if (mut && !is_cmd_approved("mii-tool")) {
            fprintf(stderr, "%s[!] Security Error: Media interface mutation with mii-tool is prohibited in view-only mode.%s\n", C_RED, C_RESET);
            return 1;
        }
    } else if (strcmp(base_cmd, "ethctl") == 0) {
        if ((strstr(stage_canon, "media-type") || strstr(stage_canon, "speed") ||
             strstr(stage_canon, "duplex") || strstr(stage_canon, "vlan") ||
             strstr(stage_canon, "enable") || strstr(stage_canon, "disable")) &&
            !is_cmd_approved("ethctl")) {
            fprintf(stderr, "%s[!] Security Error: Interface state mutation with ethctl is prohibited in view-only mode.%s\n", C_RED, C_RESET);
            return 1;
        }
    } else if (strcmp(base_cmd, "cat") == 0 || strcmp(base_cmd, "head") == 0 ||
               strcmp(base_cmd, "tail") == 0 || strcmp(base_cmd, "more") == 0 ||
               strcmp(base_cmd, "less") == 0 || strcmp(base_cmd, "grep") == 0 ||
               strcmp(base_cmd, "egrep") == 0 || strcmp(base_cmd, "fgrep") == 0 ||
               strcmp(base_cmd, "rg") == 0 || strcmp(base_cmd, "tree") == 0 ||
               strcmp(base_cmd, "ls") == 0 || strcmp(base_cmd, "dir") == 0 ||
               strcmp(base_cmd, "vdir") == 0 || strcmp(base_cmd, "du") == 0 ||
               strcmp(base_cmd, "wc") == 0 || strcmp(base_cmd, "diff") == 0 ||
               strcmp(base_cmd, "strings") == 0 || strcmp(base_cmd, "hexdump") == 0 ||
               strcmp(base_cmd, "stat") == 0 || strcmp(base_cmd, "file") == 0 ||
               strcmp(base_cmd, "cut") == 0 || strcmp(base_cmd, "column") == 0) {
        if (is_sensitive_file_access(stage_canon, argv, argc)) {
            fprintf(stderr, "%s[!] Security Error: Access to sensitive system security files is prohibited in view-only mode.%s\n", C_RED, C_RESET);
            return 1;
        }
        if (strcmp(base_cmd, "tree") == 0) {
            for (int i = 1; i < argc; i++) {
                if (argv[i][0] == '-') {
                    if (strchr(argv[i], 'o') || strchr(argv[i], 'O') || strncmp(argv[i], "--output", 8) == 0) {
                        fprintf(stderr, "%s[!] Security Error: 'tree' output file redirection (-o) is prohibited in view-only mode.%s\n", C_RED, C_RESET);
                        return 1;
                    }
                }
            }
        } else if (strcmp(base_cmd, "diff") == 0) {
            for (int i = 1; i < argc; i++) {
                if (strncmp(argv[i], "--diff", 6) == 0 || (argv[i][0] == '-' && strchr(argv[i], 'D'))) {
                    fprintf(stderr, "%s[!] Security Error: 'diff' external program execution is prohibited in view-only mode.%s\n", C_RED, C_RESET);
                    return 1;
                }
            }
        }
        if (strcmp(base_cmd, "less") == 0 || strcmp(base_cmd, "more") == 0) {
            *p_is_safe_stream = 1;
        }
    } else if (strcmp(base_cmd, "cd") == 0) {
        if (argc > 1 && check_file_path_security(argv[1])) {
            fprintf(stderr, "%s[!] Security Error: Access to sensitive system security directories is prohibited in view-only mode.%s\n", C_RED, C_RESET);
            return 1;
        }
    } else if (strcmp(base_cmd, "sort") == 0) {
        for (int i = 1; i < argc; i++) {
            const char *a = argv[i];
            if (a[0] == '-') {
                if (strchr(a, 'o') || strchr(a, 'O') ||
                    strncmp(a, "--output", 8) == 0 || strncmp(a, "--compress", 10) == 0 ||
                    strncmp(a, "--files0-from", 13) == 0 || strncmp(a, "--batch-size", 12) == 0) {
                    fprintf(stderr, "%s[!] Security Error: 'sort' file writing (-o), external compressors, and input file lists (--files0-from) are prohibited in view-only mode.%s\n", C_RED, C_RESET);
                    return 1;
                }
            }
        }
        if (is_sensitive_file_access(stage_canon, argv, argc)) {
            fprintf(stderr, "%s[!] Security Error: Access to sensitive system security files is prohibited in view-only mode.%s\n", C_RED, C_RESET);
            return 1;
        }
    } else if (strcmp(base_cmd, "uniq") == 0) {
        int non_flags = 0;
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] != '-') non_flags++;
        }
        if (non_flags >= 2) {
            fprintf(stderr, "%s[!] Security Error: 'uniq' output file redirection is prohibited in view-only mode. Use pipelines instead.%s\n", C_RED, C_RESET);
            return 1;
        }
        if (is_sensitive_file_access(stage_canon, argv, argc)) {
            fprintf(stderr, "%s[!] Security Error: Access to sensitive system security files is prohibited in view-only mode.%s\n", C_RED, C_RESET);
            return 1;
        }
    } else if (strcmp(base_cmd, "route") == 0) {
        int mut = 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "add") == 0 || strcmp(argv[i], "del") == 0 || strcmp(argv[i], "delete") == 0) {
                mut = 1;
                break;
            }
        }
        if (mut && !is_cmd_approved("route")) {
            if (is_cmd_denied("route")) {
                fprintf(stderr, "%s[!] Security Error: 'route' modification operations have been denied by host for this session.%s\n", C_RED, C_RESET);
                return 1;
            }
            fprintf(stderr, "%s[!] Security Error: 'route' modification operations (add/del) are prohibited in view-only mode.%s\n", C_RED, C_RESET);
            prompt_or_hint_approval(orig_input, "route");
            return 1;
        }
    } else if (strcmp(base_cmd, "arp") == 0) {
        int mut = 0;
        for (int i = 1; i < argc; i++) {
            const char *a = argv[i];
            if (a[0] == '-') {
                if (strpbrk(a, "sdDfF") != NULL || strcmp(a, "--delete") == 0) {
                    mut = 1;
                    break;
                }
            }
        }
        if (mut && !is_cmd_approved("arp")) {
            if (is_cmd_denied("arp")) {
                fprintf(stderr, "%s[!] Security Error: 'arp' modification operations have been denied by host for this session.%s\n", C_RED, C_RESET);
                return 1;
            }
            fprintf(stderr, "%s[!] Security Error: 'arp' modification operations (-s/-d/-f) are prohibited in view-only mode.%s\n", C_RED, C_RESET);
            prompt_or_hint_approval(orig_input, "arp");
            return 1;
        }
    } else if (strcmp(base_cmd, "wl") == 0) {
        int mut = 0;
        for (int i = 1; i < argc; i++) {
            const char *a = argv[i];
            if (strcmp(a, "down") == 0 || strcmp(a, "up") == 0 || strcmp(a, "join") == 0 ||
                strcmp(a, "disassoc") == 0 || strcmp(a, "deauth") == 0 || strcmp(a, "restart") == 0 ||
                strcmp(a, "set") == 0 || strcmp(a, "mute") == 0 || strcmp(a, "out") == 0 ||
                strcmp(a, "radio") == 0 || strcmp(a, "off") == 0 || strcmp(a, "reinit") == 0 ||
                strcmp(a, "reset") == 0 || strcmp(a, "txpower") == 0 || strcmp(a, "channel") == 0 ||
                strcmp(a, "ssid") == 0 || strcmp(a, "wep") == 0 || strcmp(a, "wpa") == 0) {
                mut = 1;
                break;
            }
        }
        if (mut && !is_cmd_approved("wl")) {
            if (is_cmd_denied("wl")) {
                fprintf(stderr, "%s[!] Security Error: 'wl' state-changing operations have been denied by host for this session.%s\n", C_RED, C_RESET);
                return 1;
            }
            fprintf(stderr, "%s[!] Security Error: 'wl' state-changing operations (down/up/radio/channel/ssid) are prohibited in view-only mode.%s\n", C_RED, C_RESET);
            prompt_or_hint_approval(orig_input, "wl");
            return 1;
        }
    } else if (strcmp(base_cmd, "nvram") == 0) {
        const char *action = sub_arg ? sub_arg : "";
        if (strcmp(action, "get") == 0) {
            for (int i = 2; i < argc; i++) {
                if (argv[i][0] == '-') continue;
                if (is_sensitive_nvram_key(argv[i])) {
                    fprintf(stderr, "%s[!] Security Error: Access to sensitive nvram credential variable '%s' is prohibited in view-only mode.%s\n", C_RED, argv[i], C_RESET);
                    return 1;
                }
            }
        } else if (strcmp(action, "show") == 0) {
            if (!is_cmd_approved("nvram")) {
                if (is_cmd_denied("nvram")) {
                    fprintf(stderr, "%s[!] Security Error: 'nvram show' has been denied by host for this session.%s\n", C_RED, C_RESET);
                    return 1;
                }
                fprintf(stderr, "%s[!] Security Error: 'nvram show' dumps all plaintext credentials. Use 'nvram get <key>' for specific configuration keys.%s\n", C_RED, C_RESET);
                prompt_or_hint_approval(orig_input, "nvram");
                return 1;
            }
        } else {
            if (!is_cmd_approved("nvram")) {
                if (is_cmd_denied("nvram")) {
                    fprintf(stderr, "%s[!] Security Error: 'nvram %s' has been denied by host for this session.%s\n", C_RED, action[0] ? action : "<empty>", C_RESET);
                    return 1;
                }
                fprintf(stderr, "%s[!] Security Error: 'nvram %s' is blocked. Only 'nvram get' is permitted in view-only mode.%s\n", C_RED, action[0] ? action : "<empty>", C_RESET);
                prompt_or_hint_approval(orig_input, "nvram");
                return 1;
            }
        }
    } else if (strcmp(base_cmd, "opkg") == 0) {
        static const char *const opkg_safe[] = {
            "list", "list-installed", "info", "find", "status", "search", "depends", "whatdepends", NULL
        };
        const char *action = NULL;
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] != '-') {
                action = argv[i];
                break;
            }
        }
        int safe = 0;
        if (action) {
            for (int i = 0; opkg_safe[i]; i++) {
                if (strcmp(action, opkg_safe[i]) == 0) {
                    safe = 1;
                    break;
                }
            }
        }
        if (!safe && !is_cmd_approved("opkg")) {
            if (is_cmd_denied("opkg")) {
                fprintf(stderr, "%s[!] Security Error: 'opkg %s' has been denied by host for this session.%s\n", C_RED, action ? action : "<empty>", C_RESET);
                return 1;
            }
            fprintf(stderr, "%s[!] Security Error: 'opkg %s' is blocked. Only read-only package queries are permitted.%s\n", C_RED, action ? action : "<empty>", C_RESET);
            prompt_or_hint_approval(orig_input, "opkg");
            return 1;
        }
    } else if (strcmp(base_cmd, "ip") == 0) {
        static const char *const ip_verbs[] = {
            "addr", "address", "route", "neigh", "link", "rule", "a", "r", "l", NULL
        };
        const char *verb = NULL;
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] != '-') {
                verb = argv[i];
                break;
            }
        }
        int is_valid_verb = 0;
        if (verb) {
            for (int i = 0; ip_verbs[i]; i++) {
                if (strcmp(verb, ip_verbs[i]) == 0) {
                    is_valid_verb = 1;
                    break;
                }
            }
        }
        if (is_valid_verb) {
            int mut = 0;
            for (int i = 1; i < argc; i++) {
                const char *a = argv[i];
                if (strcmp(a, "add") == 0 || strcmp(a, "del") == 0 ||
                    strcmp(a, "delete") == 0 || strcmp(a, "change") == 0 ||
                    strcmp(a, "replace") == 0 || strcmp(a, "set") == 0 ||
                    strcmp(a, "flush") == 0 || strcmp(a, "save") == 0 ||
                    strcmp(a, "restore") == 0) {
                    mut = 1;
                    break;
                }
            }
            if (mut && !is_cmd_approved("ip")) {
                if (is_cmd_denied("ip")) {
                    fprintf(stderr, "%s[!] Security Error: 'ip' write operations have been denied by host for this session.%s\n", C_RED, C_RESET);
                    return 1;
                }
                fprintf(stderr, "%s[!] Security Error: 'ip' write/modify operations (add/del/set/flush) are prohibited in view-only mode.%s\n", C_RED, C_RESET);
                prompt_or_hint_approval(orig_input, "ip");
                return 1;
            }
        } else {
            if (!is_cmd_approved("ip")) {
                if (is_cmd_denied("ip")) {
                    fprintf(stderr, "%s[!] Security Error: 'ip %s' has been denied by host for this session.%s\n", C_RED, verb ? verb : "<empty>", C_RESET);
                    return 1;
                }
                fprintf(stderr, "%s[!] Security Error: 'ip %s' is blocked. Only inspection verbs (addr, route, link, neigh, rule) are permitted.%s\n", C_RED, verb ? verb : "<empty>", C_RESET);
                prompt_or_hint_approval(orig_input, "ip");
                return 1;
            }
        }
    } else if (strcmp(base_cmd, "ifconfig") == 0) {
        int mut = 0;
        for (int i = 1; i < argc; i++) {
            const char *a = argv[i];
            if (strcmp(a, "up") == 0 || strcmp(a, "down") == 0 || strcmp(a, "netmask") == 0 ||
                strcmp(a, "broadcast") == 0 || strcmp(a, "pointopoint") == 0 ||
                strcmp(a, "mtu") == 0 || strcmp(a, "hw") == 0 || strcmp(a, "metric") == 0 ||
                strcmp(a, "add") == 0 || strcmp(a, "del") == 0 ||
                strcmp(a, "promisc") == 0 || strcmp(a, "-promisc") == 0 ||
                strcmp(a, "arp") == 0 || strcmp(a, "-arp") == 0) {
                mut = 1;
                break;
            }
            int d1, d2, d3, d4;
            if (sscanf(a, "%d.%d.%d.%d", &d1, &d2, &d3, &d4) == 4) {
                mut = 1;
                break;
            }
            if (strchr(a, ':') != NULL && i >= 2 && argv[1][0] != '-') {
                /* IPv6 address assignment */
                mut = 1;
                break;
            }
        }
        if (mut && !is_cmd_approved("ifconfig")) {
            if (is_cmd_denied("ifconfig")) {
                fprintf(stderr, "%s[!] Security Error: 'ifconfig' modification operations have been denied by host for this session.%s\n", C_RED, C_RESET);
                return 1;
            }
            fprintf(stderr, "%s[!] Security Error: 'ifconfig' modification operations (up/down/add/del/ip) are prohibited in view-only mode.%s\n", C_RED, C_RESET);
            prompt_or_hint_approval(orig_input, "ifconfig");
            return 1;
        }
    } else if (strcmp(base_cmd, "top") == 0) {
        int has_batch = 0;
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
                if (strchr(argv[i], 'b') || strchr(argv[i], 'n') || strchr(argv[i], 'l')) {
                    has_batch = 1;
                    break;
                }
            }
        }
        if (!has_batch) {
            *p_is_top_batch = 1;
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------------------------------------------------------
 * Execution Engine
 * ------------------------------------------------------------------------------------------------------------------------- */

static int execute_pipeline(int nstages,
                            char *stage_argv[MAX_STAGES][MAX_ARGS],
                            int stage_argc[MAX_STAGES],
                            int safe_streams[MAX_STAGES],
                            int top_batches[MAX_STAGES]) {
    /* If single stage and it is a built-in command, execute in-process */
    if (nstages == 1 && is_builtin(get_base_name(stage_argv[0][0]))) {
        return run_builtin(get_base_name(stage_argv[0][0]), stage_argc[0], stage_argv[0]);
    }

    int pipefds[MAX_STAGES - 1][2];
    for (int i = 0; i < nstages - 1; i++) {
        if (pipe(pipefds[i]) < 0) {
            perror("pipe");
            for (int j = 0; j < i; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }
            return 1;
        }
    }

    pid_t pids[MAX_STAGES];

    for (int i = 0; i < nstages; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            for (int p = 0; p < nstages - 1; p++) {
                close(pipefds[p][0]);
                close(pipefds[p][1]);
            }
            return 1;
        }

        if (pids[i] == 0) {
            /* Child process: connect pipeline fds */
            if (i > 0) {
                if (dup2(pipefds[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2");
                    _exit(127);
                }
            }
            if (i < nstages - 1) {
                if (dup2(pipefds[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    _exit(127);
                }
            }

            /* Close all pipe fds */
            for (int p = 0; p < nstages - 1; p++) {
                close(pipefds[p][0]);
                close(pipefds[p][1]);
            }

            const char *base = get_base_name(stage_argv[i][0]);

            /* If built-in in a multi-stage pipeline, run and exit */
            if (is_builtin(base)) {
                int res = run_builtin(base, stage_argc[i], stage_argv[i]);
                fflush(stdout);
                fflush(stderr);
                _exit(res);
            }

            /* Modify argv for safe streaming pagers (less, more -> cat) */
            char *exec_argv[MAX_ARGS + 4];
            int e_argc = 0;

            if (safe_streams[i]) {
                exec_argv[e_argc++] = "cat";
                for (int a = 1; a < stage_argc[i]; a++) {
                    if (stage_argv[i][a][0] != '-' && stage_argv[i][a][0] != '+') {
                        exec_argv[e_argc++] = stage_argv[i][a];
                    }
                }
            } else if (top_batches[i]) {
                exec_argv[e_argc++] = stage_argv[i][0];
#if defined(__APPLE__) || defined(__MACH__)
                exec_argv[e_argc++] = "-l";
                exec_argv[e_argc++] = "1";
                exec_argv[e_argc++] = "-n";
                exec_argv[e_argc++] = "0";
#else
                exec_argv[e_argc++] = "-b";
                exec_argv[e_argc++] = "-n";
                exec_argv[e_argc++] = "1";
#endif
                for (int a = 1; a < stage_argc[i]; a++) {
                    exec_argv[e_argc++] = stage_argv[i][a];
                }
            } else {
                for (int a = 0; a < stage_argc[i]; a++) {
                    exec_argv[e_argc++] = stage_argv[i][a];
                }
            }
            exec_argv[e_argc] = NULL;

            /* Execute directly via kernel execvp - bypassing /bin/sh */
            execvp(exec_argv[0], exec_argv);

            /* If exec fails */
            fprintf(stderr, "%s: %s\n", exec_argv[0], strerror(errno));
            _exit(127);
        }
    }

    /* Parent process: close all pipe fds */
    for (int p = 0; p < nstages - 1; p++) {
        close(pipefds[p][0]);
        close(pipefds[p][1]);
    }

    /* Wait for all children */
    int last_status = 0;
    for (int i = 0; i < nstages; i++) {
        int st = 0;
        while (waitpid(pids[i], &st, 0) < 0) {
            if (errno != EINTR) break;
        }
        if (i == nstages - 1) {
            if (WIFEXITED(st)) {
                last_status = WEXITSTATUS(st);
            } else if (WIFSIGNALED(st)) {
                last_status = 128 + WTERMSIG(st);
            }
        }
    }

    return last_status;
}

/* -------------------------------------------------------------------------------------------------------------------------
 * Main Validation Entry Point
 * ------------------------------------------------------------------------------------------------------------------------- */

static int validate_and_run(const char *raw_input) {
    if (!raw_input) return 0;
    char clean_input[MAX_LINE_LEN];
    strncpy(clean_input, raw_input, sizeof(clean_input) - 1);
    clean_input[sizeof(clean_input) - 1] = '\0';
    trim_whitespace(clean_input);
    if (!clean_input[0]) return 0;

    /* 1. Block dangerous control characters, redirection, background execution & subshells */
    if (strchr(clean_input, '\n') || strchr(clean_input, '\r')) {
        fprintf(stderr, "%s[!] Security Error: Multi-line command execution is prohibited in view-only mode.%s\n", C_RED, C_RESET);
        return 1;
    }
    if (strchr(clean_input, '>') || strchr(clean_input, '<')) {
        fprintf(stderr, "%s[!] Security Error: File redirection (>, >>, <) is prohibited in view-only mode.%s\n", C_RED, C_RESET);
        return 1;
    }
    if (strchr(clean_input, '`') || strchr(clean_input, '$')) {
        fprintf(stderr, "%s[!] Security Error: Subshell execution and variable expansion (`, $) are prohibited in view-only mode.%s\n", C_RED, C_RESET);
        return 1;
    }
    if (strchr(clean_input, ';') || strchr(clean_input, '&') || strstr(clean_input, "||")) {
        fprintf(stderr, "%s[!] Security Error: Command chaining (; && ||) and background execution (&) are prohibited. Use pipelines (|) with allowed tools.%s\n", C_RED, C_RESET);
        return 1;
    }

    /* 2. Top-level request command dispatcher */
    if (strncmp(clean_input, "request", 7) == 0 ||
        strncmp(clean_input, "req", 3) == 0 ||
        strncmp(clean_input, "ask", 3) == 0) {

        char *prefix = NULL;
        if (strncmp(clean_input, "request", 7) == 0 && (clean_input[7] == '\0' || isspace((unsigned char)clean_input[7]))) {
            prefix = clean_input + 7;
        } else if (strncmp(clean_input, "req", 3) == 0 && (clean_input[3] == '\0' || isspace((unsigned char)clean_input[3]))) {
            prefix = clean_input + 3;
        } else if (strncmp(clean_input, "ask", 3) == 0 && (clean_input[3] == '\0' || isspace((unsigned char)clean_input[3]))) {
            prefix = clean_input + 3;
        }

        if (prefix) {
            while (*prefix && isspace((unsigned char)*prefix)) prefix++;
            if (!*prefix) {
                printf("%sUsage: request <command> [arguments]%s\n", C_CYAN, C_RESET);
                printf("  Submits a real-time permission request to the router host.\n");
                printf("  Host can approve for the entire session or for a single execution.\n\n");
                return 1;
            }

            /* Extract target tool base */
            char target_cmd[MAX_LINE_LEN];
            strncpy(target_cmd, prefix, sizeof(target_cmd) - 1);
            target_cmd[sizeof(target_cmd) - 1] = '\0';
            trim_whitespace(target_cmd);

            /* Check pipeline stages inside target_cmd for first unapproved command */
            char r_stages[MAX_STAGES][MAX_LINE_LEN];
            int nr = split_pipeline(target_cmd, r_stages);
            char req_base[64] = {0};

            if (nr > 0) {
                for (int s = 0; s < nr; s++) {
                    char rtok_buf[MAX_LINE_LEN];
                    char *r_argv[MAX_ARGS];
                    int r_argc = tokenize_stage(r_stages[s], r_argv, rtok_buf);
                    if (r_argc > 0) {
                        const char *b = get_base_name(r_argv[0]);
                        static const char *const allowed_req[] = {
                            "uptime", "date", "free", "df", "ps", "top", "uname", "dmesg", "sysinfo",
                            "cpuinfo", "meminfo", "temperature", "lsmod", "netstat", "route", "arp",
                            "ping", "ping6", "traceroute", "traceroute6", "mtr", "nslookup", "dig",
                            "host", "wl", "ethtool", "mii-tool", "ethctl", "robocfg", "leases",
                            "dhcp-leases", "wifi", "ports", "logs", "help", "exit", "quit", "clear",
                            "cat", "head", "tail", "more", "less", "grep", "egrep", "fgrep", "rg",
                            "tree", "ls", "dir", "vdir", "du", "wc", "sort", "uniq", "cut", "column",
                            "tr", "diff", "strings", "stat", "file", "hexdump", "locate", "which",
                            "whereis", "echo", "printf", "who", "w", "id", "pwd", "cd", NULL
                        };
                        int is_a = 0;
                        for (int i = 0; allowed_req[i]; i++) {
                            if (strcmp(b, allowed_req[i]) == 0) {
                                is_a = 1;
                                break;
                            }
                        }
                        if (!is_a && !is_cmd_approved(b)) {
                            strncpy(req_base, b, sizeof(req_base) - 1);
                            break;
                        }
                    }
                }
            }

            if (!req_base[0]) {
                /* fallback to first token */
                char fbuf[MAX_LINE_LEN];
                char *f_argv[MAX_ARGS];
                int f_argc = tokenize_stage(target_cmd, f_argv, fbuf);
                if (f_argc > 0) {
                    strncpy(req_base, get_base_name(f_argv[0]), sizeof(req_base) - 1);
                }
            }

            return request_command_approval(target_cmd, req_base);
        }
    }

    /* 3. Split input into pipeline stages */
    char stages[MAX_STAGES][MAX_LINE_LEN];
    int nstages = split_pipeline(clean_input, stages);
    if (nstages <= 0) return 1;

    char *stage_argv[MAX_STAGES][MAX_ARGS];
    char stage_tok_bufs[MAX_STAGES][MAX_LINE_LEN];
    int stage_argc[MAX_STAGES];
    int safe_streams[MAX_STAGES];
    int top_batches[MAX_STAGES];
    memset(safe_streams, 0, sizeof(safe_streams));
    memset(top_batches, 0, sizeof(top_batches));

    /* 4. Validate all stages */
    for (int i = 0; i < nstages; i++) {
        if (validate_stage(stages[i], clean_input, stage_argv[i], &stage_argc[i],
                           stage_tok_bufs[i], &safe_streams[i], &top_batches[i]) != 0) {
            return 1;
        }
    }

    /* 5. Execute validated pipeline */
    return execute_pipeline(nstages, stage_argv, stage_argc, safe_streams, top_batches);
}

/* -------------------------------------------------------------------------------------------------------------------------
 * Main Entry Point & Shell Loop
 * ------------------------------------------------------------------------------------------------------------------------- */

int main(int argc, char *argv[]) {
    /* Sanitize dynamic linker controls, shell variables, and execution environment */
    unsetenv("LD_PRELOAD");
    unsetenv("LD_LIBRARY_PATH");
    unsetenv("LD_AUDIT");
    unsetenv("LD_DEBUG");
    unsetenv("DYLD_INSERT_LIBRARIES");
    unsetenv("DYLD_LIBRARY_PATH");
    unsetenv("DYLD_FRAMEWORK_PATH");
    unsetenv("BASH_ENV");
    unsetenv("ENV");
    unsetenv("IFS");
    unsetenv("CDPATH");
    unsetenv("GLOBIGNORE");

    /* Safe default environment */
    setenv("PATH", "/opt/bin:/opt/sbin:/bin:/usr/bin:/sbin:/usr/sbin", 1);
    setenv("PAGER", "cat", 1);
    setenv("LESSSECURE", "1", 1);
    setenv("LESS", "-M -R", 1);

    /* Non-interactive command invocation (-c "<command>") */
    if (argc >= 2 && strcmp(argv[1], "-c") == 0) {
        if (argc == 2) return 0;
        if (argc == 3) {
            return validate_and_run(argv[2]);
        }
        /* If multiple tokens follow -c, concatenate them */
        char full_cmd[MAX_LINE_LEN] = {0};
        size_t len = 0;
        for (int i = 2; i < argc; i++) {
            size_t arg_len = strlen(argv[i]);
            if (len + arg_len + 2 < sizeof(full_cmd)) {
                if (len > 0) full_cmd[len++] = ' ';
                memcpy(&full_cmd[len], argv[i], arg_len);
                len += arg_len;
                full_cmd[len] = '\0';
            }
        }
        return validate_and_run(full_cmd);
    }

    /* Interactive Login Session */
    print_banner();

    char line_buf[MAX_LINE_LEN];
    while (1) {
        printf("%stailcatzero-view:~$ %s", C_GREEN, C_RESET);
        fflush(stdout);

        if (!fgets(line_buf, sizeof(line_buf), stdin)) {
            printf("\n%sSession closed.%s\n", C_CYAN, C_RESET);
            break;
        }

        line_buf[strcspn(line_buf, "\r\n")] = '\0';
        validate_and_run(line_buf);
    }

    return 0;
}
