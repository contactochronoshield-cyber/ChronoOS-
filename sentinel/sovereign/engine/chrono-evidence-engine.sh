#!/bin/sh

set -eu

BASE_DIR="sentinel/sovereign"
EVIDENCE_DIR="$BASE_DIR/evidence"
ACQUISITION_DIR="$EVIDENCE_DIR/acquisition"
HASH_DIR="$EVIDENCE_DIR/hashes"
MANIFEST_DIR="$EVIDENCE_DIR/manifests"
CUSTODY_DIR="$EVIDENCE_DIR/chain-of-custody"

mkdir -p \
    "$ACQUISITION_DIR" \
    "$HASH_DIR" \
    "$MANIFEST_DIR" \
    "$CUSTODY_DIR"

timestamp() {
    date -u '+%Y-%m-%dT%H:%M:%SZ'
}

sha256_file() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    else
        printf '%s\n' "ERROR: SHA-256 utility not available" >&2
        exit 1
    fi
}

EVIDENCE_ID="CHR-EV-$(date -u '+%Y%m%d-%H%M%S')"
TIMESTAMP="$(timestamp)"

MANIFEST="$MANIFEST_DIR/$EVIDENCE_ID.manifest"
HASH_FILE="$HASH_DIR/$EVIDENCE_ID.sha256"
CUSTODY_FILE="$CUSTODY_DIR/$EVIDENCE_ID.custody"

printf '%s\n' "======================================"
printf '%s\n' "        CHRONO EVIDENCE ENGINE"
printf '%s\n' "======================================"
printf '%s\n' "Mode: EVIDENCE PRESERVATION"
printf '%s\n' "Original evidence: READ-ONLY"
printf '%s\n' "Hash algorithm: SHA-256"
printf '%s\n' "Evidence ID: $EVIDENCE_ID"
printf '%s\n' ""

COUNT=0

printf '%s\n' "===== EVIDENCE INVENTORY ====="

: > "$HASH_FILE"
: > "$MANIFEST"

find "$ACQUISITION_DIR" -type f -print 2>/dev/null |
while IFS= read -r FILE; do
    [ -n "$FILE" ] || continue

    HASH="$(sha256_file "$FILE")"
    SIZE="$(wc -c < "$FILE" | tr -d ' ')"

    printf '%s  %s\n' "$HASH" "$FILE" >> "$HASH_FILE"

    {
        printf 'evidence_id=%s\n' "$EVIDENCE_ID"
        printf 'timestamp=%s\n' "$TIMESTAMP"
        printf 'path=%s\n' "$FILE"
        printf 'size_bytes=%s\n' "$SIZE"
        printf 'sha256=%s\n' "$HASH"
        printf '%s\n' "---"
    } >> "$MANIFEST"

    printf '%s\n' "Evidence: $FILE"
    printf '%s\n' "SHA-256: $HASH"
    printf '%s\n' "Size: $SIZE bytes"
    printf '%s\n' ""

    COUNT=$((COUNT + 1))
done

{
    printf '%s\n' "CHRONO_CHAIN_OF_CUSTODY_V1"
    printf 'evidence_id=%s\n' "$EVIDENCE_ID"
    printf 'acquired_at=%s\n' "$TIMESTAMP"
    printf 'evidence_count=%s\n' "$COUNT"
    printf '%s\n' "originals_modified=NO"
    printf '%s\n' "hash_algorithm=SHA-256"
    printf '%s\n' "analysis_on_originals=DISABLED"
    printf '%s\n' "preservation_state=RECORDED"
} > "$CUSTODY_FILE"

printf '%s\n' "===== PRESERVATION STATE ====="
printf '%s\n' "Evidence records: $COUNT"
printf '%s\n' "Original evidence modified: NO"
printf '%s\n' "Preservation state: RECORDED"
printf '%s\n' ""

printf '%s\n' "Manifest: $MANIFEST"
printf '%s\n' "Hashes: $HASH_FILE"
printf '%s\n' "Chain of custody: $CUSTODY_FILE"
printf '%s\n' ""
printf '%s\n' "Chrono Evidence Engine: COMPLETE"

exit 0
