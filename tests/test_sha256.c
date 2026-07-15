/*
 * unit tests for the sha256 implementation
 * uses the well known nist / rfc test vectors
 * no framework just asserts a nonzero exit code means failure
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../src/sha256.h"

static int failures = 0;

static void hex_of(const uint8_t *b, size_t n, char *out) {
    static const char *d = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[i * 2]     = d[b[i] >> 4];
        out[i * 2 + 1] = d[b[i] & 0xf];
    }
    out[n * 2] = '\0';
}

static void check(const char *msg, const char *input, const char *expect) {
    uint8_t digest[SHA256_DIGEST_SIZE];
    char got[SHA256_DIGEST_SIZE * 2 + 1];
    sha256((const uint8_t *)input, strlen(input), digest);
    hex_of(digest, SHA256_DIGEST_SIZE, got);
    if (strcmp(got, expect) != 0) {
        fprintf(stderr, "FAIL %s\n  expected %s\n  got      %s\n", msg, expect, got);
        failures++;
    } else {
        fprintf(stderr, "ok   %s\n", msg);
    }
}

/* streaming update in small chunks must equal the one shot result */
static void check_streaming(void) {
    const char *input = "The quick brown fox jumps over the lazy dog";
    const char *expect =
        "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592";

    sha256_ctx ctx;
    sha256_init(&ctx);
    for (size_t i = 0; i < strlen(input); i++)
        sha256_update(&ctx, (const uint8_t *)input + i, 1);
    uint8_t digest[SHA256_DIGEST_SIZE];
    sha256_final(&ctx, digest);

    char got[SHA256_DIGEST_SIZE * 2 + 1];
    hex_of(digest, SHA256_DIGEST_SIZE, got);
    if (strcmp(got, expect) != 0) {
        fprintf(stderr, "FAIL streaming byte by byte\n  expected %s\n  got      %s\n",
                expect, got);
        failures++;
    } else {
        fprintf(stderr, "ok   streaming byte by byte\n");
    }
}

int main(void) {
    check("empty string", "",
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    check("abc", "abc",
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    check("two block message",
          "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    check_streaming();

    if (failures) {
        fprintf(stderr, "\n%d sha256 test(s) failed\n", failures);
        return 1;
    }
    fprintf(stderr, "\nall sha256 tests passed\n");
    return 0;
}
