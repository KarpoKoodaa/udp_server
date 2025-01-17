#include "../include/gbn.h"


bool gbn_process_packet (char *read, long bytes_received, int expectedseqnum)
{

    crc data[100];
    memset(data, '\0', sizeof(data));

    for (long i = 0; i < bytes_received; ++i) {
        data[i] = read[i];
    }
    data[bytes_received] = '\0';

    crc result = crcFast(data, bytes_received);

    if (result != 0) {
        return false;


    }

    int received_seq_num = (int)read[0];
    // printf("Read[0]: %c\n", read[0]);
    printf("Received seq: %d\n", received_seq_num);
    printf("Expected SEQ: %d\n", expectedseqnum);

    if (received_seq_num != expectedseqnum) {
        return false;
    }

    return true;

}


int gbn_make_packet(char *packet, uint8_t expectedseqnum)
{
    // int packet_len = -1;
    crc CRC = 0;
    char *ack = "ACK";

    char ack_message[8];
    memset(ack_message, '0', sizeof(ack_message));
    int message_len = snprintf(NULL, 0, "%c%s", expectedseqnum, ack);
    snprintf(ack_message, message_len + 1, "%c%s", expectedseqnum, ack);

    printf("ACK:%s\n", ack_message);

    CRC = crcFast((crc *)ack_message, message_len);

    printf("CRC: %x\n", CRC);

    size_t size = snprintf(NULL, 0, "%c%s%c", expectedseqnum, ack, CRC);
    snprintf(packet, size + 1, "%c%s%c", expectedseqnum, ack, CRC);
    // printf("Packet: %c\n", packet[3]);
    for (size_t i = 0; i < size; ++i) {
        printf("%zu: %hhx\t", i, packet[i]);
    }
    printf("\n");

    return  size;
    

}

