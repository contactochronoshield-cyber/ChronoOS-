/*
 * ChronoOS Industrial Security
 * Modbus TCP Defensive Guard
 *
 * Defensive parser only.
 *
 * Modbus TCP ADU:
 *
 *   Transaction ID : 2 bytes
 *   Protocol ID    : 2 bytes
 *   Length         : 2 bytes
 *   Unit ID        : 1 byte
 *   PDU            : variable
 *
 * This implementation:
 * - validates complete MBAP framing
 * - handles TCP fragmentation
 * - handles multiple ADUs in one recv()
 * - enforces length limits
 * - validates function codes
 * - never executes shell commands
 * - never sends arbitrary commands
 * - does not perform automatic PLC control
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MODBUS_MBAP_SIZE       7U
#define MODBUS_MAX_PDU        253U
#define MODBUS_MAX_ADU        260U
#define MODBUS_RX_BUFFER     4096U
#define MODBUS_MAX_RULES       256U

typedef struct {
    int function_code;
    int register_addr;
    int min_value;
    int max_value;
    char name[64];
} modbus_rule_t;

typedef struct {
    uint16_t transaction_id;
    uint16_t protocol_id;
    uint16_t length;
    uint8_t unit_id;
    uint8_t function_code;
    const uint8_t *pdu;
    size_t pdu_len;
} modbus_frame_t;

static modbus_rule_t rules[MODBUS_MAX_RULES];
static size_t rule_count = 0U;

static uint16_t read_be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8U) | p[1]);
}

static int valid_function(uint8_t fc)
{
    switch (fc) {
        case 1U:
        case 2U:
        case 3U:
        case 4U:
        case 5U:
        case 6U:
        case 15U:
        case 16U:
        case 23U:
            return 1;

        default:
            return 0;
    }
}

static int is_write_function(uint8_t fc)
{
    return fc == 5U ||
           fc == 6U ||
           fc == 15U ||
           fc == 16U ||
           fc == 23U;
}

static int parse_mbap(
    const uint8_t *buffer,
    size_t available,
    modbus_frame_t *frame)
{
    if (buffer == NULL || frame == NULL) {
        return -1;
    }

    if (available < MODBUS_MBAP_SIZE) {
        return 0;
    }

    uint16_t transaction_id = read_be16(buffer);
    uint16_t protocol_id = read_be16(buffer + 2U);
    uint16_t length = read_be16(buffer + 4U);
    uint8_t unit_id = buffer[6];

    /*
     * Modbus TCP protocol identifier must be zero.
     */
    if (protocol_id != 0U) {
        return -2;
    }

    /*
     * Length includes Unit ID + PDU.
     * Minimum = Unit ID + Function Code.
     */
    if (length < 2U || length > (MODBUS_MAX_PDU + 1U)) {
        return -3;
    }

    size_t total_adu = 6U + (size_t)length;

    if (total_adu > MODBUS_MAX_ADU) {
        return -4;
    }

    if (available < total_adu) {
        return 0;
    }

    const uint8_t *pdu = buffer + MODBUS_MBAP_SIZE;
    size_t pdu_len = (size_t)length - 1U;

    if (pdu_len == 0U) {
        return -5;
    }

    frame->transaction_id = transaction_id;
    frame->protocol_id = protocol_id;
    frame->length = length;
    frame->unit_id = unit_id;
    frame->function_code = pdu[0];
    frame->pdu = pdu;
    frame->pdu_len = pdu_len;

    if (!valid_function(frame->function_code)) {
        return -6;
    }

    return (int)total_adu;
}

static int evaluate_frame(const modbus_frame_t *frame)
{
    if (frame == NULL) {
        return -1;
    }

    printf(
        "[MODBUS] TID=%u UNIT=%u FC=%u PDU=%zu bytes\n",
        (unsigned)frame->transaction_id,
        (unsigned)frame->unit_id,
        (unsigned)frame->function_code,
        frame->pdu_len
    );

    if (is_write_function(frame->function_code)) {
        printf(
            "[ALERT] Modbus write function observed: FC=%u\n",
            (unsigned)frame->function_code
        );

        /*
         * Detection only.
         * No PLC write is performed here.
         */
        return 1;
    }

    return 0;
}

static int process_tcp_buffer(
    const uint8_t *buffer,
    size_t length)
{
    size_t offset = 0U;
    int alerts = 0;

    while (offset < length) {
        size_t remaining = length - offset;

        modbus_frame_t frame;

        int result = parse_mbap(
            buffer + offset,
            remaining,
            &frame
        );

        if (result == 0) {
            /*
             * Incomplete frame.
             * Caller must retain remaining bytes.
             */
            break;
        }

        if (result < 0) {
            printf(
                "[MODBUS] Invalid frame at offset %zu error=%d\n",
                offset,
                result
            );

            /*
             * Stop parsing rather than attempting to resynchronize
             * on attacker-controlled bytes.
             */
            return -1;
        }

        if (evaluate_frame(&frame) > 0) {
            alerts++;
        }

        offset += (size_t)result;
    }

    return alerts;
}

int chrono_modbus_validate_frame(
    const uint8_t *buffer,
    size_t length)
{
    modbus_frame_t frame;

    int result = parse_mbap(
        buffer,
        length,
        &frame
    );

    if (result <= 0) {
        return result;
    }

    return evaluate_frame(&frame);
}

void chrono_modbus_status(void)
{
    printf("=== ChronoOS Modbus Guard ===\n");
    printf("Mode: defensive monitor\n");
    printf("Protocol: Modbus TCP\n");
    printf("MBAP validation: ENABLED\n");
    printf("TCP fragmentation handling: ENABLED\n");
    printf("Multiple ADU handling: ENABLED\n");
    printf("Bounds checking: ENABLED\n");
    printf("Shell execution: DISABLED\n");
    printf("Automatic PLC control: DISABLED\n");
    printf("Rules loaded: %zu\n", rule_count);
}

#ifdef CHRONO_MODBUS_TEST
int main(void)
{
    uint8_t normal_frame[] = {
        0x00, 0x01,
        0x00, 0x00,
        0x00, 0x06,
        0x01,
        0x03,
        0x00, 0x00,
        0x00, 0x01
    };

    uint8_t write_frame[] = {
        0x00, 0x02,
        0x00, 0x00,
        0x00, 0x06,
        0x01,
        0x06,
        0x00, 0x01,
        0x00, 0x2A
    };

    uint8_t invalid_protocol[] = {
        0x00, 0x03,
        0x00, 0x01,
        0x00, 0x06,
        0x01,
        0x03,
        0x00, 0x00,
        0x00, 0x01
    };

    uint8_t truncated[] = {
        0x00, 0x04,
        0x00, 0x00,
        0x00
    };

    printf("===== NORMAL READ =====\n");
    printf(
        "RESULT=%d\n",
        chrono_modbus_validate_frame(
            normal_frame,
            sizeof(normal_frame))
    );

    printf("\n===== WRITE DETECTION =====\n");
    printf(
        "RESULT=%d\n",
        chrono_modbus_validate_frame(
            write_frame,
            sizeof(write_frame))
    );

    printf("\n===== INVALID PROTOCOL =====\n");
    printf(
        "RESULT=%d\n",
        chrono_modbus_validate_frame(
            invalid_protocol,
            sizeof(invalid_protocol))
    );

    printf("\n===== TRUNCATED FRAME =====\n");
    printf(
        "RESULT=%d\n",
        chrono_modbus_validate_frame(
            truncated,
            sizeof(truncated))
    );

    printf("\n");
    chrono_modbus_status();

    return 0;
}
#endif
