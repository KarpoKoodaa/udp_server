/**
 * 
 * Filename:    sr.h
 * 
 * Description: Selective Repeat UDP server part written in C. This implements a reliable data transfer
 *              mechanism over an unreliable UDP connection by handling packet loss, retransmissions, and acknowledgments. 
 *              In addition, delivering the received packet to upper layer from the buffer.
 *             
 * 
 * Copyright (c) 2025 Kariantti Laitala
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/
#ifndef __SR_H__
#define __SR_H__
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#include "../include/sleep.h"
#include "../include/rdn_num.h"
#include "../include/crc.h"

#define MAX_BUFFER_SIZE 50

enum SR_Packet_ACK {
    NAK = -1,
    ACK = 0
};

/**
 * @brief Structure to hold received packets in the Selective Repeat protocol.
 *
 * This struct maintains a buffer for storing received packets and tracking 
 * which packets have been successfully received.
 *
 * @param received Boolean array indicating whether each packet has been received.
 * @param data Array storing the actual packet data.
 */
typedef struct {
    bool received[MAX_BUFFER_SIZE]; 
    char data[MAX_BUFFER_SIZE];
} sr_receive_buffer_t;

/**
 * @brief Processes a received packet and verifies its integrity using CRC.
 *
 * This function extracts data from the received packet, calculates its CRC checksum, 
 * and verifies whether the packet is corrupted. If the packet passes the CRC check, 
 * the function returns the sequence number of the received packet. Otherwise, it 
 * indicates an error.
 *
 * @param read Pointer to the received packet data.
 * @param bytes_received Number of bytes received in the packet.
 * 
 * @return The sequence number of the received packet if the CRC check passes.
 * @return NAK (-1) if the CRC check fails, indicating a corrupted packet.
 */
int sr_process_packet (char *read, long bytes_received);

/**
 * @brief Constructs an acknowledgment (ACK) packet with a sequence number and CRC checksum.
 *
 * This function creates an ACK packet by combining the sequence number, 
 * acknowledgment message, and CRC checksum into a formatted string.
 *
 * @param packet A pointer to the buffer where the constructed ACK packet will be stored.
 * @param seqnum The sequence number to be included in the ACK packet.
 * 
 * @return The size of the created ACK packet.
 */
int sr_make_packet(char *packet, uint8_t seqnum);

/**
 * @brief Delivers received packets from the receive buffer to the upper layer.
 *
 * This function iterates through the receive buffer, transferring packets 
 * that have been received in order to the provided data buffer. It updates 
 * the receive buffer state accordingly and marks packets as delivered.
 *
 * @param buffer The receive buffer containing received packets.
 * @param data A pointer to the buffer where the delivered data will be stored.
 * @param recv_base The base sequence number of the first expected packet.
 * 
 * @return The updated base sequence number after delivering all available packets.
 */
int deliver_data(sr_receive_buffer_t buffer, char *data, int recv_base);

#endif
