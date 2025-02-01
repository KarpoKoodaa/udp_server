#include "../include/gbn.h"


int gbn_process_packet (char *read, long bytes_received, int expectedseqnum)
{

    crc data[100];
    memset(data, '\0', sizeof(data));

    for (long i = 0; i < bytes_received; ++i) {
        data[i] = read[i];
    }
    data[bytes_received] = '\0';

    crc result = crcFast(data, bytes_received);

    if (result != 0) {
        return CRC_NOK;
    }

    int received_seq_num = (int)read[0];
    printf("----- Packet Received Successfully -------\n");
    printf("(%d/%d) Received / Expected Sequence\n", received_seq_num, expectedseqnum);
    

    if (received_seq_num != expectedseqnum) {
        return SEQ_NOK;
    }

    return OK;

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


    CRC = crcFast((crc *)ack_message, message_len);

    size_t size = snprintf(NULL, 0, "%c%s%c", expectedseqnum, ack, CRC);
    snprintf(packet, size + 1, "%c%s%c", expectedseqnum, ack, CRC);

    return  size;
}

