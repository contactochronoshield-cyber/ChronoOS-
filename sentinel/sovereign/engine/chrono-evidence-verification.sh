#!/bin/sh

set -eu

BASE_DIR="sentinel/sovereign"
EVIDENCE_DIR="$BASE_DIR/evidence"
ACQUISITION_DIR="$EVIDENCE_DIR/acquisition"
HASH_DIR="$EVIDENCE_DIR/hashes"
REPORT_DIR="$BASE_DIR/reports/evidence"

mkdir -p "$REPORT_DIR"

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

TIMESTAMP="$(timestamp)"
REPORT="$REPORT_DIR/verification-$(date -u '+%Y%m%d-%H%M%S').report"

VERIFIED=0
FAILED=0
MISSING=0

printf '%s\n' "======================================"
printf '%s\n' "   CHRONO EVIDENCE VERIFICATION"
printf '%s\n' "======================================"
printf '%s\n' "Mode: INTEGRITY VERIFICATION"
printf '%s\n' "Original evidence: READ-ONLY"
printf '%s\n' "Hash algorithm: SHA-256"
printf '%s\n' "Timestamp: $TIMESTAMP"
printf '%s\n' ""

: > "$REPORT"

{
    printf '%s\n' "CHRONO_EVIDENCE_VERIFICATION_V1"
    printf 'verification_timestamp=%s\n' "$TIMESTAMP"
    printf 'hash_algorithm=SHA-256\n'
    printf '%s\n' "---"
} >> "$REPORT"

printf '%s\n' "===== HASH RECORDS ====="

FOUND_HASH=0

for HASH_FILE in "$HASH_DIR"/*.sha256; do
    [ -f "$HASH_FILE" ] || continue

    FOUND_HASH=1

    while IFS= read -r RECORD; do
        [ -n "$RECORD" ] || continue

        EXPECTED_HASH="$(printf '%s\n' "$RECORD" | awk '{print $1}')"
        FILE="$(printf '%s\n' "$RECORD" | sed 's/^[^ ]*  //')"

        if [ -z "$EXPECTED_HASH" ] || [ -z "$FILE" ]; then
            continue
        fi

        if [ ! -f "$FILE" ]; then
            printf '%s\n' "MISSING: $FILE"
            printf '%s\n' "status=MISSING" >> "$REPORT"
            printf 'path=%s\n' "$FILE" >> "$REPORT"
            printf '%s\n' "---" >> "$REPORT"
            MISSING=$((MISSING + 1))
            continue
        fi

        ACTUAL_HASH="$(sha256_file "$FILE")"

        if [ "$EXPECTED_HASH" = "$ACTUAL_HASH" ]; then
            printf '%s\n' "VERIFIED: $FILE"
            printf '%s\n' "SHA-256: $ACTUAL_HASH"

            {
                printf '%s\n' "status=VERIFIED"
                printf 'path=%s\n' "$FILE"
                printf 'expected_sha256=%s\n' "$EXPECTED_HASH"
                printf 'actual_sha256=%s\n' "$ACTUAL_HASH"
                printf '%s\n' "---"
            } >> "$REPORT"

            VERIFIED=$((VERIFIED + 1))
        else
            printf '%s\n' "INTEGRITY FAILURE: $FILE"
            printf '%s\n' "Expected: $EXPECTED_HASH"
            printf '%s\n' "Actual:   $ACTUAL_HASH"

            {
                printf '%s\n' "status=INTEGRITY_FAILURE"
                printf 'path=%s\n' "$FILE"
                printf 'expected_sha256=%s\n' "$EXPECTED_HASH"
                printf 'actual_sha256=%s\n' "$ACTUAL_HASH"
                printf '%s\n' "---"
            } >> "$REPORT"

            FAILED=$((FAILED + 1))
        fi

        printf '%s\n' ""
    done < "$HASH_FILE"
done

if [ "$FOUND_HASH" -eq 0 ]; then
    printf '%s\n' "No SHA-256 evidence records available."
fi

printf '%s\n' "===== VERIFICATION SUMMARY ====="
printf '%s\n' "Verified: $VERIFIED"
printf '%s\n' "Integrity failures: $FAILED"
printf '%s\n' "Missing evidence: $MISSING"

{
    printf '%s\n' "VERIFICATION_SUMMARY"
    printf 'verified=%s\n' "$VERIFIED"
    printf 'integrity_failures=%s\n' "$FAILED"
    printf 'missing=%s\n' "$MISSING"
} >> "$REPORT"

if [ "$FAILED" -gt 0 ] || [ "$MISSING" -gt 0 ]; then
    printf '%s\n' ""
    printf '%s\n' "Verification state: INTEGRITY_FAILURE"
else
    printf '%s\n' ""
    printf '%s\n' "Verification state: VERIFIED"
fi

printf '%s\n' ""
printf '%s\n' "Report: $REPORT"
printf '%s\n' ""
printf '%s\n' "Chrono Evidence Verification: COMPLETE"

exit 0
