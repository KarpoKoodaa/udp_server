
#include "../include/rdt.h"

crc process_packet (char *read, long bytes_received, Rdt_variables* vars)
{
    if (rand_number() <= vars->drop_probability) {
        printf("\033[0;31m");
        printf("Packet dropped\n");
        printf("\033[0m");
        return true;
    }
    // TODO: Remove else 
    else {
    // Add delay   
        if (rand_number() <= vars->delay_probability) {
            printf("\033[0;31m");
            printf("Delay added\n");
            printf("\033[0m");
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
    
    for (long i = 0; i < bytes_received; ++i) {
        data[i] = read[i];
    }
    data[bytes_received] = '\0';

    printf("READ[0] = %d\n", read[0]);
    if (vars->rdt == 22) {
        if(read[0] == 0) vars->seq = 0;
        else if (read[0] == 1) vars->seq = 1;
    }
    
    if (vars->rdt == 30) {
        if (vars->last_seq == vars->seq) {
            // Duplicate
            printf("\033[0;31m");
            printf("Duplicate packet\n");
            printf("\033[0m");
            vars->seq = vars->last_seq;
        } 
    
        else {
             if (read[0] == 0) vars->seq = 0;
             else if (read[0] == 1) vars->seq = 1;
        }
    }

    printf("seq = %d\n", vars->seq);
    crc result = crcFast(data, bytes_received);

    return result;
               
} /* process_packet */


int make_packet(char *packet, int version, uint8_t seq, int result) {

    printf("Packet seq: %d\n", seq);
    if (seq > 1) {
        return -1;     
    }
    
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

    if (version == 20 || version == 21) {
        snprintf(packet, sizeof(packet), "%s%c", ack, CRC);
        printf("Packet is %s\n", packet);
        packet_len = snprintf(packet, sizeof(packet),"%s%c", ack, CRC);

    }
    else if (version == 22 || version == 30) {
        snprintf(packet, sizeof(packet), "%c%s%c", seq, ack, CRC);
        packet_len = snprintf(packet, sizeof(packet), "%c%s%c", seq, ack, CRC);            

    }
    printf("Packet len: %d\n", packet_len);
    return packet_len;
    
} /* make_packet */
