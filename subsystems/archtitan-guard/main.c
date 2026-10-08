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
    return apply_enforcement_full();
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
#include "hashes.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SHA256_BLOCK_SIZE 32
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t data[64];
    uint32_t datalen;
} SHA256_CTX;

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
    uint32_t a,b,c,d,e,f,g,h,i,t1,t2,m[64];
    for (i=0;i<16;i++) {
        m[i] = ((uint32_t)data[i*4]<<24) | ((uint32_t)data[i*4+1]<<16) |
               ((uint32_t)data[i*4+2]<<8)  | ((uint32_t)data[i*4+3]);
    }
    for (i=16;i<64;i++) {
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[i-16];
    }
    a=ctx->state[0]; b=ctx->state[1]; c=ctx->state[2]; d=ctx->state[3];
    e=ctx->state[4]; f=ctx->state[5]; g=ctx->state[6]; h=ctx->state[7];
    for (i=0;i<64;i++) {
        t1 = h + EP1(e) + CH(e,f,g) + sha256_k[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    ctx->state[0]+=a; ctx->state[1]+=b; ctx->state[2]+=c; ctx->state[3]+=d;
    ctx->state[4]+=e; ctx->state[5]+=f; ctx->state[6]+=g; ctx->state[7]+=h;
}

static void sha256_init(SHA256_CTX *ctx) {
    ctx->datalen=0; ctx->bitlen=0;
    ctx->state[0]=0x6a09e667; ctx->state[1]=0xbb67ae85; ctx->state[2]=0x3c6ef372; ctx->state[3]=0xa54ff53a;
    ctx->state[4]=0x510e527f; ctx->state[5]=0x9b05688c; ctx->state[6]=0x1f83d9ab; ctx->state[7]=0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len) {
    for (size_t i=0;i<len;i++) {
        ctx->data[ctx->datalen++] = data[i];
        if (ctx->datalen==64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
    uint32_t i = ctx->datalen;
    ctx->data[i++] = 0x80;
    if (i>56) {
        while (i<64) ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        i=0;
    }
    while (i<56) ctx->data[i++] = 0x00;
    ctx->bitlen += ctx->datalen*8;
    ctx->data[63]=ctx->bitlen; ctx->data[62]=ctx->bitlen>>8; ctx->data[61]=ctx->bitlen>>16; ctx->data[60]=ctx->bitlen>>24;
    ctx->data[59]=ctx->bitlen>>32; ctx->data[58]=ctx->bitlen>>40; ctx->data[57]=ctx->bitlen>>48; ctx->data[56]=ctx->bitlen>>56;
    sha256_transform(ctx, ctx->data);
    for (i=0;i<4;i++) {
        hash[i]   = (ctx->state[0]>>(24-i*8))&0x000000ff;
        hash[i+4] = (ctx->state[1]>>(24-i*8))&0x000000ff;
        hash[i+8] = (ctx->state[2]>>(24-i*8))&0x000000ff;
        hash[i+12]= (ctx->state[3]>>(24-i*8))&0x000000ff;
        hash[i+16]= (ctx->state[4]>>(24-i*8))&0x000000ff;
        hash[i+20]= (ctx->state[5]>>(24-i*8))&0x000000ff;
        hash[i+24]= (ctx->state[6]>>(24-i*8))&0x000000ff;
        hash[i+28]= (ctx->state[7]>>(24-i*8))&0x000000ff;
    }
}

static int sha256_file(const char *path, char *out_hex) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    SHA256_CTX ctx;
    sha256_init(&ctx);
    unsigned char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        sha256_update(&ctx, buf, n);
    }
    fclose(f);
    unsigned char hash[SHA256_BLOCK_SIZE];
    sha256_final(&ctx, hash);
    for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
        sprintf(out_hex + i*2, "%02x", hash[i]);
    }
    out_hex[SHA256_BLOCK_SIZE*2] = '\0';
    return 1;
}

static int hex_equal(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int verify_file_checksum(const char *path, const char *expected_hex, const char *backup_path) {
    char actual[SHA256_BLOCK_SIZE*2 + 1];
    if (sha256_file(path, actual) && hex_equal(actual, expected_hex)) {
        return 1;
    }
    if (backup_path && sha256_file(backup_path, actual) && hex_equal(actual, expected_hex)) {
        FILE *src = fopen(backup_path, "rb");
        if (!src) return 0;
        FILE *dst = fopen(path, "wb");
        if (!dst) { fclose(src); return 0; }
        unsigned char buf[8192];
        size_t n;
        int ok = 1;
        while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
            if (fwrite(buf, 1, n, dst) != n) { ok = 0; break; }
        }
        fclose(src);
        fclose(dst);
        if (ok) {
            struct stat st;
            if (stat(backup_path, &st) == 0) {
                chmod(path, st.st_mode);
            }
            char msg[512];
            snprintf(msg, sizeof(msg), "Restored tampered file from backup: %s", path);
            log_msg(msg);
            return 1;
        }
    }
    char msg[512];
    snprintf(msg, sizeof(msg), "Checksum mismatch for critical file: %s", path);
    log_msg(msg);
    return 0;
}





static int apply_enforcement(void) {
    return apply_enforcement_full();
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
#include "hashes.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SHA256_BLOCK_SIZE 32
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t data[64];
    uint32_t datalen;
} SHA256_CTX;

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
    uint32_t a,b,c,d,e,f,g,h,i,t1,t2,m[64];
    for (i=0;i<16;i++) {
        m[i] = ((uint32_t)data[i*4]<<24) | ((uint32_t)data[i*4+1]<<16) |
               ((uint32_t)data[i*4+2]<<8)  | ((uint32_t)data[i*4+3]);
    }
    for (i=16;i<64;i++) {
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[i-16];
    }
    a=ctx->state[0]; b=ctx->state[1]; c=ctx->state[2]; d=ctx->state[3];
    e=ctx->state[4]; f=ctx->state[5]; g=ctx->state[6]; h=ctx->state[7];
    for (i=0;i<64;i++) {
        t1 = h + EP1(e) + CH(e,f,g) + sha256_k[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    ctx->state[0]+=a; ctx->state[1]+=b; ctx->state[2]+=c; ctx->state[3]+=d;
    ctx->state[4]+=e; ctx->state[5]+=f; ctx->state[6]+=g; ctx->state[7]+=h;
}

static void sha256_init(SHA256_CTX *ctx) {
    ctx->datalen=0; ctx->bitlen=0;
    ctx->state[0]=0x6a09e667; ctx->state[1]=0xbb67ae85; ctx->state[2]=0x3c6ef372; ctx->state[3]=0xa54ff53a;
    ctx->state[4]=0x510e527f; ctx->state[5]=0x9b05688c; ctx->state[6]=0x1f83d9ab; ctx->state[7]=0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len) {
    for (size_t i=0;i<len;i++) {
        ctx->data[ctx->datalen++] = data[i];
        if (ctx->datalen==64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
    uint32_t i = ctx->datalen;
    ctx->data[i++] = 0x80;
    if (i>56) {
        while (i<64) ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        i=0;
    }
    while (i<56) ctx->data[i++] = 0x00;
    ctx->bitlen += ctx->datalen*8;
    ctx->data[63]=ctx->bitlen; ctx->data[62]=ctx->bitlen>>8; ctx->data[61]=ctx->bitlen>>16; ctx->data[60]=ctx->bitlen>>24;
    ctx->data[59]=ctx->bitlen>>32; ctx->data[58]=ctx->bitlen>>40; ctx->data[57]=ctx->bitlen>>48; ctx->data[56]=ctx->bitlen>>56;
    sha256_transform(ctx, ctx->data);
    for (i=0;i<4;i++) {
        hash[i]   = (ctx->state[0]>>(24-i*8))&0x000000ff;
        hash[i+4] = (ctx->state[1]>>(24-i*8))&0x000000ff;
        hash[i+8] = (ctx->state[2]>>(24-i*8))&0x000000ff;
        hash[i+12]= (ctx->state[3]>>(24-i*8))&0x000000ff;
        hash[i+16]= (ctx->state[4]>>(24-i*8))&0x000000ff;
        hash[i+20]= (ctx->state[5]>>(24-i*8))&0x000000ff;
        hash[i+24]= (ctx->state[6]>>(24-i*8))&0x000000ff;
        hash[i+28]= (ctx->state[7]>>(24-i*8))&0x000000ff;
    }
}

static int sha256_file(const char *path, char *out_hex) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    SHA256_CTX ctx;
    sha256_init(&ctx);
    unsigned char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        sha256_update(&ctx, buf, n);
    }
    fclose(f);
    unsigned char hash[SHA256_BLOCK_SIZE];
    sha256_final(&ctx, hash);
    for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
        sprintf(out_hex + i*2, "%02x", hash[i]);
    }
    out_hex[SHA256_BLOCK_SIZE*2] = '\0';
    return 1;
}

static int hex_equal(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int verify_file_checksum(const char *path, const char *expected_hex, const char *backup_path) {
    char actual[SHA256_BLOCK_SIZE*2 + 1];
    if (sha256_file(path, actual) && hex_equal(actual, expected_hex)) {
        return 1;
    }
    if (backup_path && sha256_file(backup_path, actual) && hex_equal(actual, expected_hex)) {
        FILE *src = fopen(backup_path, "rb");
        if (!src) return 0;
        FILE *dst = fopen(path, "wb");
        if (!dst) { fclose(src); return 0; }
        unsigned char buf[8192];
        size_t n;
        int ok = 1;
        while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
            if (fwrite(buf, 1, n, dst) != n) { ok = 0; break; }
        }
        fclose(src);
        fclose(dst);
        if (ok) {
            struct stat st;
            if (stat(backup_path, &st) == 0) {
                chmod(path, st.st_mode);
            }
            char msg[512];
            snprintf(msg, sizeof(msg), "Restored tampered file from backup: %s", path);
            log_msg(msg);
            return 1;
        }
    }
    char msg[512];
    snprintf(msg, sizeof(msg), "Checksum mismatch for critical file: %s", path);
    log_msg(msg);
    return 0;
}



static int apply_enforcement_full(void) {
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

    if (!verify_file_checksum("/usr/lib/archtitan/archtitan-guard",
                              HASH_archtitan_guard,
                              "/usr/share/archtitan/immutable/archtitan-guard")) {
        return 3;
    }
    if (!verify_file_checksum("/etc/pacman.d/hooks/archtitan-session-guard.hook",
                              HASH_archtitan_session_guard_hook,
                              "/usr/share/archtitan/immutable/archtitan-session-guard.hook")) {
        return 3;
    }
    if (!verify_file_checksum("/etc/systemd/system/archtitan-immutable-guard.service",
                              HASH_archtitan_immutable_guard_service,
                              "/usr/share/archtitan/immutable/archtitan-immutable-guard.service")) {
        return 3;
    }
    return 0;
}
