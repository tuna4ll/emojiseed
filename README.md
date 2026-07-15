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

## Web demo

The encode/decode core lives in `src/core.c` with no I/O, so the browser demo
compiles the exact same code to WebAssembly — no logic is reimplemented in
JavaScript. Randomness in the browser comes from `crypto.getRandomValues`, and
everything runs locally; nothing leaves the page.

```sh
make wasm            # needs emscripten (emcc), writes web/emojiseed.js + web/wordlist.js
# then open web/index.html in a browser
```

CI builds the demo on every push and deploys `web/` to GitHub Pages.

## Layout

| Path                  | What                                             |
|-----------------------|--------------------------------------------------|
| `src/core.c`          | pure entropy ↔ emoji-index math (shared)         |
| `src/sha256.c`        | SHA-256 for the checksum                         |
| `src/emojiseed.c`     | CLI: args, wordlist, hex, `/dev/urandom`         |
| `web/emojiseed_wasm.c`| WebAssembly bindings over the core               |
| `tests/`              | unit tests (SHA-256, core) + CLI integration     |

## Tests

```sh
make test            # unit tests + cli integration tests
make test-unit       # sha256 and core known-answer / roundtrip tests
make test-cli        # drives the built binary
```
