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
    char *seq = &read[0];
    int received_seq_num = atoi(seq);
    printf("Received seq: %d\n", received_seq_num);

    if (received_seq_num != expectedseqnum) {
        return false;
    }

    return true;

}


int gbn_make_packet(char *packet, uint8_t expectedseqnum)
{
    int packet_len = -1;
    crc CRC = 0;
    char *ack = "ACK";

    char ack_message[5];
    int message_len = snprintf(NULL, 0, "%c%s", expectedseqnum, ack);
    snprintf(ack_message, message_len, "%c%s", expectedseqnum, ack);

    CRC = crcFast((crc *)ack_message, message_len);

    packet_len = snprintf(NULL, 0, "%c%s%c", expectedseqnum, ack, CRC);
    snprintf(packet, packet_len, "%c%s%c", expectedseqnum, ack, CRC);

    return packet_len;
    

}

