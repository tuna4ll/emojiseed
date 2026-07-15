/*
 * pure entropy <-> word index math shared by the cli the tests and wasm
 * see core.h for the contract
 */
#include "core.h"
#include "sha256.h"

/* bit helpers msb first over a byte buffer */
static int get_bit(const uint8_t *buf, size_t i) {
    return (buf[i / 8] >> (7 - (i % 8))) & 1;
}
static void set_bit(uint8_t *buf, size_t i, int v) {
    if (v) buf[i / 8] |= (uint8_t)(1u << (7 - (i % 8)));
    else   buf[i / 8] &= (uint8_t)~(1u << (7 - (i % 8)));
}

int emojiseed_encode(const uint8_t *ent, size_t ent_len, int *idx) {
    size_t ent_bits = ent_len * 8;
    if (ent_bits != 128 && ent_bits != 256) return -1;
    size_t cs_bits = ent_bits / 32;                 /* 4 or 8 */
    size_t total   = ent_bits + cs_bits;            /* 132 or 264 */
    if (total % EMOJISEED_BITS_PER_WORD != 0) return -1;

    uint8_t hash[SHA256_DIGEST_SIZE];
    sha256(ent, ent_len, hash);

    uint8_t bits[34] = {0};                          /* ceil of 264 over 8 is 33 */
    for (size_t i = 0; i < ent_len; i++) bits[i] = ent[i];
    for (size_t i = 0; i < cs_bits; i++)             /* append leading hash bits */
        set_bit(bits, ent_bits + i, get_bit(hash, i));

    int nwords = (int)(total / EMOJISEED_BITS_PER_WORD);
    for (int w = 0; w < nwords; w++) {
        int v = 0;
        for (int b = 0; b < EMOJISEED_BITS_PER_WORD; b++)
            v = (v << 1) | get_bit(bits, (size_t)w * EMOJISEED_BITS_PER_WORD + b);
        idx[w] = v;
    }
    return nwords;
}

int emojiseed_decode(const int *idx, int nwords,
                     uint8_t *ent, size_t *ent_len, int *checksum_ok) {
    if (nwords != 11 && nwords != 22) return -1;
    for (int w = 0; w < nwords; w++)
        if (idx[w] < 0 || idx[w] >= EMOJISEED_WORDLIST_SIZE) return -1;

    size_t total   = (size_t)nwords * EMOJISEED_BITS_PER_WORD;   /* 132 or 264 */
    size_t ent_bits = total * 32 / 33;                           /* 128 or 256 */
    size_t cs_bits  = ent_bits / 32;
    if (ent_bits + cs_bits != total) return -1;

    uint8_t bits[34] = {0};
    for (int w = 0; w < nwords; w++)
        for (int b = 0; b < EMOJISEED_BITS_PER_WORD; b++)
            set_bit(bits, (size_t)w * EMOJISEED_BITS_PER_WORD + b,
                    (idx[w] >> (EMOJISEED_BITS_PER_WORD - 1 - b)) & 1);

    size_t nbytes = ent_bits / 8;
    for (size_t i = 0; i < nbytes; i++) ent[i] = bits[i];
    *ent_len = nbytes;

    /* verify checksum */
    uint8_t hash[SHA256_DIGEST_SIZE];
    sha256(ent, nbytes, hash);
    int ok = 1;
    for (size_t i = 0; i < cs_bits; i++)
        if (get_bit(bits, ent_bits + i) != get_bit(hash, i)) { ok = 0; break; }
    *checksum_ok = ok;
    return 0;
}
