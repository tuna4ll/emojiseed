#ifndef EMOJISEED_CORE_H
#define EMOJISEED_CORE_H

/*
 * pure entropy <-> word index math for emojiseed
 * no i/o no wordlist no randomness so it links cleanly into
 * the cli the tests and the wasm build alike
 *
 * a word index is a value in 0..4095 that maps to one emoji
 * the caller owns the emoji <-> index mapping (the wordlist)
 */
#include <stddef.h>
#include <stdint.h>

#define EMOJISEED_BITS_PER_WORD 12
#define EMOJISEED_WORDLIST_SIZE 4096   /* 2^12 */
#define EMOJISEED_MAX_WORDS     22     /* a 256 bit key */
#define EMOJISEED_MAX_ENT       32     /* bytes, 256 bit */

/*
 * encode entropy bytes into word indices using the bip39 checksum rule.
 * ent_len must be 16 (128 bit -> 11 words) or 32 (256 bit -> 22 words).
 * idx must have room for EMOJISEED_MAX_WORDS ints.
 * returns the number of words written or -1 on unsupported ent_len.
 */
int emojiseed_encode(const uint8_t *ent, size_t ent_len, int *idx);

/*
 * decode word indices back into entropy and verify the checksum.
 * nwords must be 11 (128 bit) or 22 (256 bit).
 * every idx value must be in 0..4095.
 * on success writes the entropy to ent (needs EMOJISEED_MAX_ENT bytes),
 * sets *ent_len to the byte count and *checksum_ok to 1 or 0.
 * returns 0 on success or -1 on a bad word count or out of range index.
 */
int emojiseed_decode(const int *idx, int nwords,
                     uint8_t *ent, size_t *ent_len, int *checksum_ok);

#endif /* EMOJISEED_CORE_H */
