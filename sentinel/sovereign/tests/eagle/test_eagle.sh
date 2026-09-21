#!/usr/bin/env bash

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
TEST_DIR="$ROOT/var/eagle-test"
BIN="$TEST_DIR/chrono-eagle-test"

fail()
{
    echo "[FAIL] $1"
    rm -rf "$TEST_DIR"
    exit 1
}

rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"

cat > "$TEST_DIR/eagle_test.c" <<'C'
#include <stdio.h>
#include <string.h>

#include "chrono_eagle.h"

static int test_legacy(void)
{
    eagle_camera_t camera = {0};

    strcpy(camera.id, "EAGLE-CAM-LEGACY-001");
    strcpy(camera.vendor, "LegacyVendor");
    strcpy(camera.model, "Legacy-Camera");
    strcpy(camera.firmware, "2018.04");

    camera.onvif_detected = 1;
    camera.tls_verified = 0;
    camera.authentication_verified = 0;
    camera.rtsp_detected = 1;

    camera.optical_health = 1;
    camera.alignment_health = 1;
    camera.vibration_health = 1;

    if (eagle_verify(&camera) != 0)
        return 1;

    if (camera.generation != EAGLE_GENERATION_LEGACY)
        return 2;

    if (camera.state != EAGLE_STATE_WARNING)
        return 3;

    printf("[PASS] Legacy camera classified correctly\n");

    return 0;
}

static int test_modern(void)
{
    eagle_camera_t camera = {0};

    strcpy(camera.id, "EAGLE-CAM-MODERN-001");
    strcpy(camera.vendor, "ModernVendor");
    strcpy(camera.model, "Modern-Camera");
    strcpy(camera.firmware, "2026.09");

    camera.onvif_detected = 1;
    camera.tls_verified = 1;
    camera.authentication_verified = 1;
    camera.rtsp_detected = 1;

    camera.optical_health = 1;
    camera.alignment_health = 1;
    camera.vibration_health = 1;

    if (eagle_verify(&camera) != 0)
        return 1;

    if (camera.generation != EAGLE_GENERATION_MODERN)
        return 2;

    if (camera.state != EAGLE_STATE_VERIFIED)
        return 3;

    printf("[PASS] Modern camera classified correctly\n");

    return 0;
}

static int test_physical_incident(void)
{
    eagle_camera_t camera = {0};

    strcpy(camera.id, "EAGLE-CAM-TOWER-001");
    strcpy(camera.vendor, "TowerVendor");
    strcpy(camera.model, "Tower-Camera");
    strcpy(camera.firmware, "2025.11");

    camera.onvif_detected = 1;
    camera.tls_verified = 1;
    camera.authentication_verified = 1;
    camera.rtsp_detected = 1;

    camera.optical_health = 1;
    camera.alignment_health = 3;
    camera.vibration_health = 3;

    if (eagle_verify(&camera) != 0)
        return 1;

    if (camera.state != EAGLE_STATE_INCIDENT)
        return 2;

    printf("[PASS] Physical incident detected\n");

    return 0;
}

int main(void)
{
    if (eagle_init() != 0)
        return 1;

    if (test_legacy() != 0)
        return 2;

    if (test_modern() != 0)
        return 3;

    if (test_physical_incident() != 0)
        return 4;

    printf("[OK] Chrono Eagle test passed\n");

    return 0;
}
C

gcc -Wall -Wextra -Werror \
    -I"$ROOT/core/eagle" \
    "$ROOT/core/eagle/chrono_eagle.c" \
    "$TEST_DIR/eagle_test.c" \
    -o "$BIN" \
    || fail "compilación de Águila falló"

"$BIN" || fail "test funcional de Águila falló"

rm -rf "$TEST_DIR"

echo "[OK] Chrono Eagle test completed"
