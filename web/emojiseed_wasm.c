/*
 * webassembly bindings for the emojiseed core
 * thin wrappers so the browser demo runs the exact same encode / decode
 * code as the cli no logic is reimplemented in javascript
 *
 * the emoji <-> index mapping (the wordlist) lives in javascript
 * these functions only move bits so they take and return word indices
 */
#include <emscripten.h>
#include <stdint.h>
#include <stddef.h>
#include "../src/core.h"

/*
 * encode ent_len entropy bytes at ent into word indices at idx_out.
 * idx_out must have room for EMOJISEED_MAX_WORDS int32 values.
 * returns the number of words or -1 on an unsupported length.
 */
EMSCRIPTEN_KEEPALIVE
int es_encode(const uint8_t *ent, int ent_len, int *idx_out) {
    return emojiseed_encode(ent, (size_t)ent_len, idx_out);
}

/*
 * decode nwords indices at idx into entropy bytes at ent_out.
 * writes the checksum result (0 or 1) to *ok_out.
 * returns the entropy byte count or -1 on a bad word count or index.
 */
EMSCRIPTEN_KEEPALIVE
int es_decode(const int *idx, int nwords, uint8_t *ent_out, int *ok_out) {
    size_t ent_len = 0;
    int ok = 0;
    if (emojiseed_decode(idx, nwords, ent_out, &ent_len, &ok) != 0)
        return -1;
    *ok_out = ok;
    return (int)ent_len;
}
