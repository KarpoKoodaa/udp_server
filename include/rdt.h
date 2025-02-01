/******************************************************************************
  * @file           : rdt.h
  * @brief          : TODO 
******************************************************************************/

#ifndef __RDT_H__
#define __RDT_H__
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#include "../include/sleep.h"
#include "../include/rdn_num.h"
#include "../include/crc.h"

/**
 * @brief Stores parameters for reliable data transfer (RDT).
 * 
 * This struct holds various probabilities for simulating network conditions 
 * (such as packet drops, errors, and delays) as well as sequence numbers and 
 * the RDT version used.
 */
typedef struct Rdt_variables
{
    float drop_probability;   /**< Probability of a packet being dropped. */
    float delay_probability;  /**< Probability of a packet experiencing a delay. */
    float error_probability;  /**< Probability of a packet being corrupted. */
    uint16_t delay_ms;        /**< Delay duration in milliseconds if delay occurs. */
    uint8_t seq;              /**< Current sequence number of the packet. */
    int8_t last_seq;          /**< Last acknowledged sequence number. */
    uint16_t rdt;             /**< Reliable data transfer version (1.0, 2.0, 2.1, 2.2, or 3.0). */
} Rdt_variables;


crc process_packet (char *read, long bytes_received, Rdt_variables* vars);

/**
 * @brief Creates a packet based on the specified rdt_vars.rdt version, sequence number, and result.
 *
 * This function constructs a packet with a specific format depending on the rdt version provided.
 * The packet contains acknowledgment (ACK/NAK) and a CRC value for integrity checking.
 *
 * @param[out] packet Pointer to a buffer where the created packet will be stored.
 * @param[in]  version Protocol version (e.g., 20, 21, 22, or 30 ) that determines the packet format.
 * @param[in]  seq Sequence number (0 or 1) used in the packet. Must not exceed 1.
 * @param[in]  result Result status (0 for success, non-zero for failure) influencing the ACK/NAK decision.
 * 
 * @return Length of the created packet on success, 
 *          and `-1` if error occurred
 * @note For `version == 22` and `seq == 1`, the CRC is hardcoded to 0x69 as per test app behavior.
 *
 * ### Packet Formats:
 * - **Version 20, 21**:
 *   - On success (`result == 0`): `"ACK<CRC>"`
 *   - On failure (`result != 0`): `"NAK<CRC>"`
 * - **Version 22, 30**:
 *   - On success (`result == 0`): `"<seq>ACK<CRC>"`
 *   - On failure (`result != 0`): `"<seq>NAK<CRC>"`
 *
 * ### Error Handling:
 * - If error occurs, the function returns `-1` 
 */
int make_packet(char *packet, int version, uint8_t seq, int result);

#endif /* __RDT_H__ */
