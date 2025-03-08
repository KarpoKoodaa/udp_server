
#include "../include/rdt.h"

#define RED     "\033[1;31m"
#define ORANGE  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define RESET   "\033[0m"


crc process_packet (char *read, long bytes_received, Rdt_variables* vars)
{
    if (rand_number() <= vars->drop_probability) {
        printf(RED "------- Packet Dropped -------\n\n" RESET);
        return true;
    }
    // TODO: Remove else 
    else {
    // Add delay   
        if (rand_number() <= vars->delay_probability) {
            printf(RED "------- Delay Added -------\n\n" RESET);
            msleep(vars->delay_ms);
        }
        // Add bit error
        if (rand_number() <= vars->error_probability) {
            char mask = 0x2;
            read[bytes_received-2] = read[bytes_received-2] ^ mask;
        }
    }

    crc data[100];
    memset(data, '\0', sizeof(data));
    
    memcpy(data, read, bytes_received);
    data[bytes_received] = '\0';

    // printf("READ[0] = %d\n", read[0]);
    if (vars->rdt == 22) {
        if(read[0] == 0) vars->seq = 0;
        else if (read[0] == 1) vars->seq = 1;
    }
    
    if (vars->rdt == 30) {
        if (vars->last_seq == vars->seq) {
            // Duplicate
            printf(RED "------- Duplicate Packet -------\n\n" RESET);
            vars->seq = vars->last_seq;
        } 
    
        else {
             if (read[0] == 0) vars->seq = 0;
             else if (read[0] == 1) vars->seq = 1;
        }
    }

    // printf("seq = %d\n", vars->seq);
    crc result = crcFast(data, bytes_received);

    return result;
               
} /* process_packet */


int make_packet(char *packet, int version, uint8_t seq, int result) {

    int packet_len = -1; // Length of created packet
    uint8_t CRC = 0;    // CRC for packet
    if (result == 0 && seq == 1) {
        // This is strange. The test app responses ACK with CRC 69, if sequence is 1
        CRC = 0x69;
    }
    else if (result == 0) {
        CRC = 0x7f;
    }
    else if (result == 1){
        CRC = 0x12;
    }

    char *ack = (result == 0 ? "ACK" : "NAK");

    // If rdt version is 2.0 or 2.1, no seq number needed with ACK packet.
    if (version == 20 || version == 21) {
        size_t size = strlen(ack) + 2;  // Adding 2 as one for CRC and other for null terminator
        snprintf(packet, size, "%s%c", ack, CRC);
        packet_len = snprintf(packet, size,"%s%c", ack, CRC);

    }
    // rdt version 2.2 and 3.0 needs seq number with ACK packet
    else if (version == 22 || version == 30) {
        size_t size = strlen(ack) + 3;  // Adding 3 as one for SEQ, one for CRC and for null terminator 
        snprintf(packet, size, "%c%s%c", seq, ack, CRC);
        packet_len = snprintf(packet, size, "%c%s%c", seq, ack, CRC);            

    }

    return packet_len;
    
} /* make_packet */
