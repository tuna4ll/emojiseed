# emojiseed

BIP39 style mnemonics but with emojis instead of words. Encodes a private key
into emojis and decodes it back.

## Idea

- 4096 emojis so 12 bits per emoji (BIP39 uses 2048 words = 11 bits).
- Same checksum rule: append `ENT/32` leading bits of `SHA-256(entropy)`, then
  slice the bit stream into 12 bit groups.

| Entropy | Emojis |
|--------:|-------:|
| 128 bit | 11     |
| 256 bit | 22     |

A full 256 bit key = 22 emojis.

## Build

Needs gcc and make (Linux / WSL / macOS). Randomness comes from `/dev/urandom`.

```sh
make
```

## Usage

```sh
./emojiseed gen                 # random 256 bit key
./emojiseed gen --bits 128
./emojiseed encode <hex>        # 16 or 32 bytes of hex
./emojiseed decode "😀 🚀 ..."   # back to hex + checksum check
```

## Wordlist

`data/emojis.txt` has exactly 4096 emojis, one per line, order matters. Override
with `--wordlist <path>` or `EMOJISEED_WORDLIST`.
