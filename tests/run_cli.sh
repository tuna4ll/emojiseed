#!/bin/sh
# integration tests that drive the built emojiseed binary
# run from the repo root so data/emojis.txt resolves
# a nonzero exit code means at least one test failed

set -u

BIN="${BIN:-./emojiseed}"
fails=0

if [ ! -x "$BIN" ]; then
    echo "error binary not found at $BIN run make first" >&2
    exit 1
fi

pass() { echo "ok   $1"; }
fail() { echo "FAIL $1" >&2; fails=$((fails + 1)); }

# encode <hex> then decode the emojis back must return the same hex
roundtrip() {
    hex="$1"
    emojis=$("$BIN" encode "$hex" 2>/dev/null)
    if [ -z "$emojis" ]; then
        fail "roundtrip encode $hex produced no output"
        return
    fi
    back=$("$BIN" decode "$emojis" 2>/dev/null)
    if [ "$back" = "$hex" ]; then
        pass "roundtrip $hex"
    else
        fail "roundtrip $hex got $back"
    fi
}

# a 128 bit key roundtrips to 11 emojis
roundtrip "00000000000000000000000000000000"
roundtrip "ffffffffffffffffffffffffffffffff"
roundtrip "0123456789abcdef0123456789abcdef"
# 256 bit keys roundtrip to 22 emojis
roundtrip "0000000000000000000000000000000000000000000000000000000000000000"
roundtrip "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
roundtrip "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"

# encode output must have the right emoji count
n=$("$BIN" encode "00000000000000000000000000000000" 2>/dev/null | wc -w)
[ "$n" -eq 11 ] && pass "128 bit encodes to 11 emojis" || fail "128 bit gave $n emojis"
n=$("$BIN" encode "0000000000000000000000000000000000000000000000000000000000000000" 2>/dev/null | wc -w)
[ "$n" -eq 22 ] && pass "256 bit encodes to 22 emojis" || fail "256 bit gave $n emojis"

# uppercase hex must decode to the same lowercase bytes
a=$("$BIN" encode "0123456789ABCDEF0123456789ABCDEF" 2>/dev/null)
b=$("$BIN" encode "0123456789abcdef0123456789abcdef" 2>/dev/null)
[ "$a" = "$b" ] && pass "hex is case insensitive" || fail "case sensitivity mismatch"

# gen produces a decodable phrase and reports valid checksum
phrase=$("$BIN" gen --bits 128 2>/dev/null)
if "$BIN" decode "$phrase" >/dev/null 2>&1; then
    pass "gen 128 output decodes with valid checksum"
else
    fail "gen 128 output failed to decode"
fi
phrase=$("$BIN" gen 2>/dev/null)
if "$BIN" decode "$phrase" >/dev/null 2>&1; then
    pass "gen default 256 output decodes with valid checksum"
else
    fail "gen default output failed to decode"
fi

# error cases must exit nonzero
"$BIN" encode "zz" >/dev/null 2>&1 && fail "invalid hex accepted" || pass "invalid hex rejected"
"$BIN" encode "00" >/dev/null 2>&1 && fail "wrong length accepted" || pass "wrong entropy length rejected"
"$BIN" decode "notanemoji" >/dev/null 2>&1 && fail "unknown emoji accepted" || pass "unknown emoji rejected"
"$BIN" gen --bits 200 >/dev/null 2>&1 && fail "bad bits accepted" || pass "bad --bits rejected"

# a phrase with a tampered checksum must be reported invalid exit code 2
good=$("$BIN" encode "00000000000000000000000000000000" 2>/dev/null)
# swap the last emoji for the first one to break the checksum
last=$(printf '%s' "$good" | awk '{print $NF}')
first=$(printf '%s' "$good" | awk '{print $1}')
if [ "$first" != "$last" ]; then
    tampered=$(printf '%s' "$good" | sed "s/$last\$/$first/")
    "$BIN" decode "$tampered" >/dev/null 2>&1
    rc=$?
    [ "$rc" -eq 2 ] && pass "tampered checksum detected" || fail "tampered checksum not detected rc=$rc"
else
    pass "tampered checksum skipped identical tokens"
fi

echo
if [ "$fails" -ne 0 ]; then
    echo "$fails cli test(s) failed" >&2
    exit 1
fi
echo "all cli tests passed"
