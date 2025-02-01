/******************************************************************************
  * @file           : gbn.h
  * @brief          : TODO 
******************************************************************************/

#ifndef __GBN_H__
#define __GBN_H__
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#include "../include/sleep.h"
#include "../include/rdn_num.h"
#include "../include/crc.h"

/**
 * @brief Represents the status of a received packet in the GBN protocol.
 * 
 * This enum defines possible outcomes when processing a received packet, 
 * including success, sequence number mismatch, and CRC errors.
 */
enum Packet_status {
    OK,       /**< Packet processed successfully. */
    SEQ_NOK,  /**< Sequence number mismatch. */
    CRC_NOK   /**< CRC error detected in the packet. */
};


/**
 * @brief Processes a received packet and performs error checking.
 * 
 * This function takes a received packet, verifies its integrity using CRC, 
 * and checks if the sequence number matches the expected sequence number.
 * 
 * @param read Pointer to the received data buffer.
 * @param bytes_received The number of bytes received in the packet.
 * @param expectedseqnum The expected sequence number of the packet.
 * @return int 
 *         - `OK` if the packet is valid and the sequence number matches.
 *         - `CRC_NOK` if there is a CRC error.
 *         - `SEQ_NOK` if the sequence number does not match the expected one.
 */
int gbn_process_packet (char *read, long bytes_received, int expectedseqnum);


/**
 * @brief Constructs an acknowledgment (ACK) packet with a sequence number and CRC.
 * 
 * This function creates an ACK packet containing the expected sequence number, 
 * the "ACK" message, and a CRC checksum for error detection. The resulting packet 
 * is stored in the provided buffer.
 * 
 * @param packet Pointer to the buffer where the constructed packet will be stored.
 * @param expectedseqnum The expected sequence number to include in the packet.
 * @return int The size of the generated packet.
 */
int gbn_make_packet(char *packet, uint8_t expectedseqnum);

#endif /* __GBN_H__ */
