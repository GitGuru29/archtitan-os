#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <libgen.h>

#define MAX_ARGS 4096

static const char *blocked_names[] = {
    "gdm", "gdm3", "lightdm", "lightdm-webkit2-greeter", "lightdm-gtk-greeter",
    "lxdm", "xdm", "ly", "emptty", "slim", "greetd", "lemurs", "entrance",
    "tuigreet", "wlgreet", "agreety", "regreet", "qtgreet", "cosmic-greeter",
    "sddm-wayland-git",
    NULL
};

static void log_msg(const char *msg) {
    fprintf(stderr, "[ArchTitan Guard] %s\n", msg);
}

static char *strtolower_dup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *res = malloc(len + 1);
    if (!res) return NULL;
    for (size_t i = 0; i < len; i++) {
        res[i] = tolower((unsigned char)s[i]);
    }
    res[len] = '\0';
    return res;
}

static int is_blocked_name(const char *pkg) {
    if (!pkg || pkg[0] == '\0') return 0;
    char *n = strtolower_dup(pkg);
    if (!n) return 0;
    char *at = strchr(n, '@');
    if (at) *at = '\0';
    for (int i = 0; blocked_names[i]; i++) {
        if (strcmp(n, blocked_names[i]) == 0) {
            free(n);
            return 1;
        }
    }
    free(n);
    return 0;
}

static int package_provides_dm(const char *pkg) {
    if (!pkg) return 0;
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "pacman -Si --print-format %%p %s 2>/dev/null", pkg);
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    char buf[8192];
    int found = 0;
    while (fgets(buf, sizeof(buf), fp)) {
        char *lower = strtolower_dup(buf);
        if (lower) {
            if (strstr(lower, "systemd-display-manager")) {
                found = 1;
            }
            free(lower);
        }
        if (found) break;
    }
    pclose(fp);
    return found;
}

static int pretransaction(int argc, char **argv) {
    char *pkgs[MAX_ARGS];
    int npkgs = 0;
    for (int i = 0; i < MAX_ARGS; i++) pkgs[i] = NULL;

    for (int i = 2; i < argc && npkgs < MAX_ARGS - 1; i++) {
        pkgs[npkgs++] = strdup(argv[i]);
    }

    if (npkgs == 0) {
        char *line = NULL;
        size_t len = 0;
        ssize_t n;
        while ((n = getline(&line, &len, stdin)) != -1 && npkgs < MAX_ARGS - 1) {
            char *p = line;
            while (*p) {
                while (isspace((unsigned char)*p)) p++;
                if (*p == '\0' || *p == '\n' || *p == '\r') break;
                char *start = p;
                while (*p && !isspace((unsigned char)*p)) p++;
                char saved = *p;
                if (saved) {
                    *p = '\0';
                    p++;
                }
                pkgs[npkgs++] = strdup(start);
                if (saved == '\0') break;
            }
        }
        if (line) free(line);
    }

    int blocked = 0;
    for (int i = 0; i < npkgs && !blocked; i++) {
        if (!pkgs[i]) continue;
        if (is_blocked_name(pkgs[i]) || package_provides_dm(pkgs[i])) {
            char msg[2048];
            snprintf(msg, sizeof(msg),
                     "Refusing to install/modify '%s' (SDDM + Hyprland is the only supported display/session stack).",
                     pkgs[i]);
            log_msg(msg);
            blocked = 1;
        }
    }

    for (int i = 0; i < npkgs; i++) {
        if (pkgs[i]) free(pkgs[i]);
    }

    return blocked ? 1 : 0;
}

static void ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        mkdir(path, 0755);
    }
}

static int is_symlink_to(const char *linkpath, const char *target) {
    char buf[4096];
    ssize_t n = readlink(linkpath, buf, sizeof(buf) - 1);
    if (n < 0) return 0;
    buf[n] = '\0';
    return strcmp(buf, target) == 0;
}

static int set_symlink_atomic(const char *linkpath, const char *target) {
    char linkdup[4096];
    strncpy(linkdup, linkpath, sizeof(linkdup) - 1);
    linkdup[sizeof(linkdup) - 1] = '\0';
    char *d = dirname(linkdup);
    ensure_dir(d);
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s.archtitan-guard-tmp", linkpath);
    unlink(tmp);
    if (symlink(target, tmp) != 0) return 0;
    if (rename(tmp, linkpath) != 0) {
        unlink(tmp);
        return 0;
    }
    return 1;
}

static int apply_enforcement(void) {
    const char *dm_service = "/etc/systemd/system/display-manager.service";
    const char *expected_dm = "/usr/lib/systemd/system/sddm.service";
    if (!is_symlink_to(dm_service, expected_dm)) {
        if (set_symlink_atomic(dm_service, expected_dm)) {
            log_msg("Restored display-manager.service symlink to sddm.service");
        } else {
            log_msg("Failed to restore display-manager.service symlink");
            return 2;
        }
    }
    const char *mask_targets[] = {
        "gdm.service", "lightdm.service", "lxdm.service", "xdm.service",
        "ly.service", "greetd.service", "emptty.service", "slim.service",
        "plasma-sddm-helper.service", NULL
    };
    ensure_dir("/etc/systemd/system");
    for (int i = 0; mask_targets[i]; i++) {
        char path[4096];
        snprintf(path, sizeof(path), "/etc/systemd/system/%s", mask_targets[i]);
        if (!is_symlink_to(path, "/dev/null")) {
            set_symlink_atomic(path, "/dev/null");
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        log_msg("Unknown command");
        return 2;
    }
    const char *cmd = argv[1];
    if (strcmp(cmd, "pretransaction") == 0) {
        return pretransaction(argc, argv);
    } else if (strcmp(cmd, "verify") == 0) {
        return apply_enforcement();
    } else if (strcmp(cmd, "apply") == 0 || strcmp(cmd, "enforce") == 0) {
        return apply_enforcement();
    } else if (strcmp(cmd, "version") == 0) {
        printf("archtitan-guard 0.1.0\n");
        return 0;
    } else {
        log_msg("Unknown command");
        return 2;
    }
}
