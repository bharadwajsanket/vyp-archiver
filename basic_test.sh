#!/usr/bin/env bash
# =============================================================================
# tests/basic_test.sh — VYP Archive Tool integration tests
# Run from the project root: bash tests/basic_test.sh
# =============================================================================

set -uo pipefail

VYPACK="${VYPACK:-./vypack}"
WORKDIR="$(mktemp -d)"
PASS=0
FAIL=0

# Colours (disabled when not a terminal)
if [ -t 1 ]; then
    GREEN='\033[0;32m'; RED='\033[0;31m'; RESET='\033[0m'
else
    GREEN=''; RED=''; RESET=''
fi

# --------------------------------------------------------------------------- #
ok()   { echo -e "${GREEN}PASS${RESET} $1"; PASS=$((PASS+1)); }
fail() { echo -e "${RED}FAIL${RESET} $1"; FAIL=$((FAIL+1)); }

assert_contains() {
    local label="$1" needle="$2" haystack="$3"
    if echo "$haystack" | grep -qF "$needle"; then
        ok "$label"
    else
        fail "$label  (expected: '$needle')"
        echo "     output was: $haystack"
    fi
}

assert_not_contains() {
    local label="$1" needle="$2" haystack="$3"
    if echo "$haystack" | grep -qF "$needle"; then
        fail "$label  (did not expect: '$needle')"
        echo "     output was: $haystack"
    else
        ok "$label"
    fi
}

assert_exit_ok() {
    local label="$1" code="$2"
    [ "$code" -eq 0 ] && ok "$label" || fail "$label  (exit $code)"
}

assert_exit_fail() {
    local label="$1" code="$2"
    [ "$code" -ne 0 ] && ok "$label" || fail "$label  (expected non-zero exit)"
}

# --------------------------------------------------------------------------- #
cd "$WORKDIR"

ARCHIVE="test.vyp"

echo "=== Working in $WORKDIR ==="
echo ""

# -- Create test data --------------------------------------------------------
echo "Hello, VYP!" > hello.txt
printf "col1,col2\n1,2\n3,4\n" > data.csv
dd if=/dev/urandom bs=1024 count=64 of=binary.bin 2>/dev/null
BINARY_MD5=$(md5sum binary.bin | awk '{print $1}')

# --------------------------------------------------------------------------- #
echo "--- init ---"
OUT=$("$VYPACK" init "$ARCHIVE")
assert_contains "init: creates archive"  "Archive created" "$OUT"
assert_exit_ok  "init: exit 0" $?

# --------------------------------------------------------------------------- #
echo "--- test on fresh archive ---"
OUT=$("$VYPACK" test "$ARCHIVE")
assert_contains "test: fresh archive OK" "Archive OK" "$OUT"

# --------------------------------------------------------------------------- #
echo "--- add ---"
OUT=$("$VYPACK" add hello.txt "$ARCHIVE")
assert_contains "add hello.txt" "Added hello.txt" "$OUT"

OUT=$("$VYPACK" add data.csv "$ARCHIVE")
assert_contains "add data.csv" "Added data.csv" "$OUT"

OUT=$("$VYPACK" add binary.bin "$ARCHIVE")
assert_contains "add binary.bin" "Added binary.bin" "$OUT"

# --------------------------------------------------------------------------- #
echo "--- duplicate prevention ---"
OUT=$("$VYPACK" add hello.txt "$ARCHIVE" 2>&1) && STATUS=$? || STATUS=$?
assert_exit_fail "duplicate: non-zero exit" $STATUS
assert_contains  "duplicate: error message" "already exists" "$OUT"

# --------------------------------------------------------------------------- #
echo "--- list ---"
OUT=$("$VYPACK" list "$ARCHIVE")
assert_contains "list: file count"  "Files: 3"    "$OUT"
assert_contains "list: hello.txt"   "hello.txt"   "$OUT"
assert_contains "list: data.csv"    "data.csv"    "$OUT"
assert_contains "list: binary.bin"  "binary.bin"  "$OUT"

# --------------------------------------------------------------------------- #
echo "--- test after adds ---"
OUT=$("$VYPACK" test "$ARCHIVE")
assert_contains "test: after adds OK" "Archive OK" "$OUT"

# --------------------------------------------------------------------------- #
echo "--- extract single ---"
rm hello.txt
OUT=$("$VYPACK" extract hello.txt "$ARCHIVE")
assert_contains  "extract: message"  "Extracted hello.txt" "$OUT"
assert_exit_ok   "extract: exit 0"   $?
CONTENT=$(cat hello.txt)
assert_contains  "extract: content"  "Hello, VYP!" "$CONTENT"

# --------------------------------------------------------------------------- #
echo "--- extract: file not found ---"
OUT=$("$VYPACK" extract nosuchfile.txt "$ARCHIVE" 2>&1) && STATUS=$? || STATUS=$?
assert_exit_fail "extract missing: non-zero exit" $STATUS
assert_contains  "extract missing: error msg" "not found" "$OUT"

# --------------------------------------------------------------------------- #
echo "--- extractall ---"
rm data.csv binary.bin
OUT=$("$VYPACK" extractall "$ARCHIVE")
assert_contains "extractall: data.csv"   "Extracted data.csv"   "$OUT"
assert_contains "extractall: binary.bin" "Extracted binary.bin" "$OUT"

# Binary round-trip integrity
EXTRACTED_MD5=$(md5sum binary.bin | awk '{print $1}')
if [ "$BINARY_MD5" = "$EXTRACTED_MD5" ]; then
    ok "binary integrity: MD5 matches"
else
    fail "binary integrity: MD5 mismatch (original=$BINARY_MD5 extracted=$EXTRACTED_MD5)"
fi

# --------------------------------------------------------------------------- #
echo "--- delete ---"
OUT=$("$VYPACK" delete data.csv "$ARCHIVE")
assert_contains "delete: message" "Deleted data.csv" "$OUT"
assert_exit_ok  "delete: exit 0" $?

OUT=$("$VYPACK" list "$ARCHIVE")
assert_contains     "delete: count updated" "Files: 2"  "$OUT"
assert_not_contains "delete: entry gone"    "data.csv"  "$OUT"
assert_contains     "delete: others remain" "hello.txt" "$OUT"

# Archive still valid after delete
OUT=$("$VYPACK" test "$ARCHIVE")
assert_contains "test: after delete OK" "Archive OK" "$OUT"

# --------------------------------------------------------------------------- #
echo "--- delete: file not found ---"
OUT=$("$VYPACK" delete nosuchfile.txt "$ARCHIVE" 2>&1) && STATUS=$? || STATUS=$?
assert_exit_fail "delete missing: non-zero exit" $STATUS
assert_contains  "delete missing: error msg" "not found" "$OUT"

# --------------------------------------------------------------------------- #
echo "--- test: corrupted archive ---"
echo "garbage data here" > corrupt.vyp
OUT=$("$VYPACK" test corrupt.vyp 2>&1)
assert_contains "test corrupted: output" "Archive corrupted" "$OUT"

# --------------------------------------------------------------------------- #
echo "--- invalid archive operations ---"
OUT=$("$VYPACK" list corrupt.vyp 2>&1) && STATUS=$? || STATUS=$?
assert_exit_fail "invalid archive list: non-zero exit" $STATUS

# --------------------------------------------------------------------------- #
echo ""
echo "============================================"
echo "  Results: ${PASS} passed, ${FAIL} failed"
echo "============================================"

# Cleanup
cd /
rm -rf "$WORKDIR"

[ "$FAIL" -eq 0 ] && exit 0 || exit 1
