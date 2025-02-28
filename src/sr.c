#include "../include/sr.h"


int sr_process_packet (char *read, long bytes_received)
{

    crc data[100];
    memset(data, '\0', sizeof(data));

    for (long i = 0; i < bytes_received; ++i) {
        data[i] = read[i];
    }
    data[bytes_received] = '\0';

    crc result = crcFast(data, bytes_received);

    if (result != 0) {
        return -2;
    }

    int received_seq_num = (int)read[0];
    printf("----- Packet Received Successfully -------\n");
    // Now there is now expected sequence number, so need to consider hot do this
    printf("%d Received\n", received_seq_num);

    return received_seq_num;

}


int sr_make_packet(char *packet, uint8_t seqnum)
{
    // int packet_len = -1;
    crc CRC = 0;
    char *ack = "ACK";

    char ack_message[8];
    memset(ack_message, '0', sizeof(ack_message));
    int message_len = snprintf(NULL, 0, "%c%s", seqnum, ack);
    snprintf(ack_message, message_len + 1, "%c%s", seqnum, ack);


    CRC = crcFast((crc *)ack_message, message_len);

    size_t size = snprintf(NULL, 0, "%c%s%c", seqnum, ack, CRC);
    snprintf(packet, size + 1, "%c%s%c", seqnum, ack, CRC);

    return  size;
}

int deliver_data(sr_receive_buffer_t buffer, char *data, int recv_base) 
{
    int base = recv_base;

    printf("----- Delivering Packets to Upper Layer -------\n");
    while(buffer.received[base]) {
        data[base-1] = buffer.data[base];
        printf("idx: %d, char: %c\n", base-1, data[base-1]);
        buffer.received[base] = false;    // chaging to false
        base++; 
    }
    
    data[base] = '\0';
    printf("Data: %s\n", data);
    return base;

}
