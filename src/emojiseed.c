/*
 * emojiseed
 * bip39 style mnemonic encoding using emojis instead of words
 *
 * wordlist 4096 emojis so 12 bits per emoji since 2^12 is 4096
 * checksum is the bip39 rule
 * cs is ENT over 32 leading bits of sha256 of the entropy
 * the entropy plus checksum bitstream is sliced into 12 bit groups
 * each group picks one emoji
 *
 * with 12 bit words ENT plus ENT over 32 divides by 12 only for
 *   ENT 128 gives 132 bits so 11 emojis
 *   ENT 256 gives 264 bits so 22 emojis which is a full private key
 *
 * commands
 *   emojiseed gen    with optional --bits 128 or 256
 *   emojiseed encode hex
 *   emojiseed decode "emoji emoji ..."
 *
 * wordlist path is --wordlist path or EMOJISEED_WORDLIST env
 * otherwise data/emojis.txt in the current directory
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "core.h"

#define WORDLIST_SIZE EMOJISEED_WORDLIST_SIZE

/* copy a string since strdup is not in plain c11 */
static char *dup_str(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

/* csprng reads from dev urandom */
static int secure_random(uint8_t *buf, size_t len) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return -1;
    size_t got = fread(buf, 1, len, f);
    fclose(f);
    return got == len ? 0 : -1;
}

/* wordlist */
typedef struct {
    char *words[WORDLIST_SIZE];   /* utf8 strings with no trailing space */
    int   count;
} wordlist_t;

static void wordlist_free(wordlist_t *wl) {
    for (int i = 0; i < wl->count; i++) free(wl->words[i]);
    wl->count = 0;
}

static int wordlist_load(wordlist_t *wl, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error cannot open wordlist %s\n", path);
        return -1;
    }
    wl->count = 0;
    char line[256];
    while (wl->count < WORDLIST_SIZE && fgets(line, sizeof(line), f)) {
        /* strip trailing cr lf space tab */
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r' ||
                         line[n - 1] == ' '  || line[n - 1] == '\t'))
            line[--n] = '\0';
        if (n == 0) continue;            /* skip blank lines */
        char *dup = malloc(n + 1);
        if (!dup) { fclose(f); return -1; }
        memcpy(dup, line, n + 1);
        wl->words[wl->count++] = dup;
    }
    fclose(f);
    if (wl->count != WORDLIST_SIZE) {
        fprintf(stderr, "error wordlist must have exactly %d entries but found %d\n",
                WORDLIST_SIZE, wl->count);
        wordlist_free(wl);
        return -1;
    }
    return 0;
}

/* find emoji token and return index or -1 */
static int wordlist_index(const wordlist_t *wl, const char *token) {
    for (int i = 0; i < wl->count; i++)
        if (strcmp(wl->words[i], token) == 0) return i;
    return -1;
}

/* hex helpers */
static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
/* parse hex string into bytes return byte count or -1 on error */
static int hex_to_bytes(const char *hex, uint8_t *out, size_t out_cap) {
    size_t len = strlen(hex);
    if (len % 2 != 0) return -1;
    size_t nbytes = len / 2;
    if (nbytes > out_cap) return -1;
    for (size_t i = 0; i < nbytes; i++) {
        int hi = hex_val(hex[i * 2]);
        int lo = hex_val(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return -1;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return (int)nbytes;
}
static void bytes_to_hex(const uint8_t *b, size_t n, char *out) {
    static const char *d = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[i * 2]     = d[b[i] >> 4];
        out[i * 2 + 1] = d[b[i] & 0xf];
    }
    out[n * 2] = '\0';
}

/* commands */
static void print_emojis(const wordlist_t *wl, const int *idx, int n) {
    for (int i = 0; i < n; i++)
        printf("%s%s", wl->words[idx[i]], (i + 1 < n) ? " " : "\n");
}

static int cmd_encode(const wordlist_t *wl, const char *hex) {
    uint8_t ent[32];
    int n = hex_to_bytes(hex, ent, sizeof(ent));
    if (n < 0) {
        fprintf(stderr, "error invalid hex\n");
        return 1;
    }
    if (n != 16 && n != 32) {
        fprintf(stderr, "error entropy must be 16 bytes 128 bit or 32 bytes 256 bit but got %d bytes\n", n);
        return 1;
    }
    int idx[EMOJISEED_MAX_WORDS];
    int nwords = emojiseed_encode(ent, (size_t)n, idx);
    if (nwords < 0) { fprintf(stderr, "error unsupported entropy size\n"); return 1; }
    print_emojis(wl, idx, nwords);
    return 0;
}

static int cmd_gen(const wordlist_t *wl, int bits) {
    if (bits != 128 && bits != 256) {
        fprintf(stderr, "error --bits must be 128 or 256\n");
        return 1;
    }
    size_t ent_len = (size_t)bits / 8;
    uint8_t ent[32];
    if (secure_random(ent, ent_len) != 0) {
        fprintf(stderr, "error secure rng failed\n");
        return 1;
    }
    char hex[65];
    bytes_to_hex(ent, ent_len, hex);
    int idx[EMOJISEED_MAX_WORDS];
    int nwords = emojiseed_encode(ent, ent_len, idx);
    fprintf(stderr, "entropy %d bit %s\n", bits, hex);
    print_emojis(wl, idx, nwords);
    return 0;
}

static int cmd_decode(const wordlist_t *wl, const char *phrase) {
    /* split on whitespace */
    int idx[64];
    int nwords = 0;
    char *buf = dup_str(phrase);
    if (!buf) return 1;
    for (char *tok = strtok(buf, " \t\r\n"); tok; tok = strtok(NULL, " \t\r\n")) {
        if (nwords >= (int)(sizeof(idx) / sizeof(idx[0]))) {
            fprintf(stderr, "error too many emojis\n"); free(buf); return 1;
        }
        int i = wordlist_index(wl, tok);
        if (i < 0) {
            fprintf(stderr, "error emoji not in wordlist %s\n", tok);
            free(buf); return 1;
        }
        idx[nwords++] = i;
    }
    free(buf);

    if (nwords != 11 && nwords != 22) {
        fprintf(stderr, "error expected 11 or 22 emojis but got %d\n", nwords);
        return 1;
    }

    uint8_t ent[EMOJISEED_MAX_ENT];
    size_t ent_len = 0;
    int ok = 0;
    if (emojiseed_decode(idx, nwords, ent, &ent_len, &ok) != 0) {
        fprintf(stderr, "error bit length mismatch\n");
        return 1;
    }

    char hex[65];
    bytes_to_hex(ent, ent_len, hex);
    printf("%s\n", hex);
    fprintf(stderr, "checksum %s %zu bit entropy\n",
            ok ? "ok" : "invalid", ent_len * 8);
    return ok ? 0 : 2;
}

/* cli */
static void usage(void) {
    fprintf(stderr,
        "emojiseed bip39 style emoji mnemonics 4096 emojis 12 bits each\n\n"
        "usage\n"
        "  emojiseed gen    [--bits 128|256]\n"
        "  emojiseed encode <hex>\n"
        "  emojiseed decode \"<emoji emoji ...>\"\n\n"
        "options\n"
        "  --wordlist <path>   emoji list default EMOJISEED_WORDLIST or data/emojis.txt\n");
}

int main(int argc, char **argv) {
    const char *wordlist_path = getenv("EMOJISEED_WORDLIST");
    if (!wordlist_path) wordlist_path = "data/emojis.txt";

    /* collect positional args and pull out flags */
    const char *pos[8];
    int npos = 0;
    int bits = 256;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--wordlist") == 0 && i + 1 < argc) {
            wordlist_path = argv[++i];
        } else if (strcmp(argv[i], "--bits") == 0 && i + 1 < argc) {
            bits = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(); return 0;
        } else if (npos < (int)(sizeof(pos) / sizeof(pos[0]))) {
            pos[npos++] = argv[i];
        }
    }
    if (npos == 0) { usage(); return 1; }

    wordlist_t wl;
    if (wordlist_load(&wl, wordlist_path) != 0) return 1;

    int rc;
    if (strcmp(pos[0], "gen") == 0) {
        rc = cmd_gen(&wl, bits);
    } else if (strcmp(pos[0], "encode") == 0 && npos >= 2) {
        rc = cmd_encode(&wl, pos[1]);
    } else if (strcmp(pos[0], "decode") == 0 && npos >= 2) {
        rc = cmd_decode(&wl, pos[1]);
    } else {
        usage();
        rc = 1;
    }
    wordlist_free(&wl);
    return rc;
}
