/*
 * unit tests for the pure encode / decode core
 * checks structural invariants known answer vectors and the
 * encode then decode roundtrip no framework nonzero exit means failure
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../src/core.h"

static int failures = 0;

static void expect(int cond, const char *msg) {
    if (cond) {
        fprintf(stderr, "ok   %s\n", msg);
    } else {
        fprintf(stderr, "FAIL %s\n", msg);
        failures++;
    }
}

/* encode then decode must return the original entropy with a valid checksum */
static void roundtrip(const uint8_t *ent, size_t ent_len, const char *label) {
    int idx[EMOJISEED_MAX_WORDS];
    int nwords = emojiseed_encode(ent, ent_len, idx);
    int expect_words = (ent_len == 16) ? 11 : 22;
    if (nwords != expect_words) {
        fprintf(stderr, "FAIL %s wrong word count %d\n", label, nwords);
        failures++;
        return;
    }
    uint8_t out[EMOJISEED_MAX_ENT];
    size_t out_len = 0;
    int ok = 0;
    int rc = emojiseed_decode(idx, nwords, out, &out_len, &ok);
    if (rc != 0 || out_len != ent_len || !ok || memcmp(out, ent, ent_len) != 0) {
        fprintf(stderr, "FAIL %s roundtrip rc=%d len=%zu ok=%d\n",
                label, rc, out_len, ok);
        failures++;
        return;
    }
    fprintf(stderr, "ok   %s roundtrip\n", label);
}

int main(void) {
    uint8_t zero16[16] = {0};
    uint8_t zero32[32] = {0};
    uint8_t ff16[16], ff32[32];
    memset(ff16, 0xff, sizeof(ff16));
    memset(ff32, 0xff, sizeof(ff32));
    uint8_t seq32[32];
    for (int i = 0; i < 32; i++) seq32[i] = (uint8_t)i;

    /* word counts */
    int idx[EMOJISEED_MAX_WORDS];
    expect(emojiseed_encode(zero16, 16, idx) == 11, "128 bit encodes to 11 words");
    expect(emojiseed_encode(zero32, 32, idx) == 22, "256 bit encodes to 22 words");

    /* unsupported entropy sizes are rejected */
    uint8_t tmp[32] = {0};
    expect(emojiseed_encode(tmp, 8,  idx) == -1, "encode rejects 8 byte entropy");
    expect(emojiseed_encode(tmp, 20, idx) == -1, "encode rejects 20 byte entropy");
    expect(emojiseed_encode(tmp, 24, idx) == -1, "encode rejects 24 byte entropy");

    /* known answer vectors computed independently with python hashlib */
    emojiseed_encode(zero16, 16, idx);
    int leading_zero_128 = 1;
    for (int i = 0; i < 10; i++) if (idx[i] != 0) leading_zero_128 = 0;
    expect(leading_zero_128, "zero-128 first 10 words are zero");
    expect(idx[10] == 3, "zero-128 checksum word equals 3");

    emojiseed_encode(zero32, 32, idx);
    int leading_zero_256 = 1;
    for (int i = 0; i < 21; i++) if (idx[i] != 0) leading_zero_256 = 0;
    expect(leading_zero_256, "zero-256 first 21 words are zero");
    expect(idx[21] == 102, "zero-256 checksum word equals 102");

    /* roundtrips */
    roundtrip(zero16, 16, "zero-128");
    roundtrip(ff16,   16, "ones-128");
    roundtrip(zero32, 32, "zero-256");
    roundtrip(ff32,   32, "ones-256");
    roundtrip(seq32,  32, "seq-256");

    /* decode rejects bad word counts and out of range indices */
    uint8_t out[EMOJISEED_MAX_ENT];
    size_t out_len = 0;
    int ok = 0;
    int bad[12] = {0};
    expect(emojiseed_decode(bad, 12, out, &out_len, &ok) == -1,
           "decode rejects 12 words");
    int good11[11] = {0};
    expect(emojiseed_decode(good11, 11, out, &out_len, &ok) == 0,
           "decode accepts 11 words");
    int oor[11] = {0};
    oor[0] = 4096;                 /* one past the last valid index */
    expect(emojiseed_decode(oor, 11, out, &out_len, &ok) == -1,
           "decode rejects out of range index");

    /* a tampered checksum word must clear checksum_ok */
    emojiseed_encode(zero16, 16, idx);
    idx[10] ^= 1;                  /* flip the checksum word */
    ok = 1;
    emojiseed_decode(idx, 11, out, &out_len, &ok);
    expect(ok == 0, "tampered checksum word reports invalid");

    if (failures) {
        fprintf(stderr, "\n%d core test(s) failed\n", failures);
        return 1;
    }
    fprintf(stderr, "\nall core tests passed\n");
    return 0;
}
