/******************************************
 * 
 * Filename:    udp_client.c
 * 
 * Description: udp_client for university exercise.
 *              Based on Lewis Van Winkle's "Hands-on Network Programming with C" book
 * 
 * Copyright (c) 2024 Kariantti Laitala
 * Permission tba
 *******************************************/

// Standard Headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Networking Headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

// Local Headers
#include "../include/crc.h"

void timeout_alarm(__attribute__((unused))int ignore);
int make_packet (uint8_t next_sequence, char data, crc packet_crc, char *packet);

#define ISVALIDSOCKET(s)    ((s) >= 0)
#define CLOSESOCKET(s)      close(s)
#define SOCKET              int
#define GETSOCKETERRNO()    (errno)

#define DEFAULT_PORT        "6666"
#define MAXTRIES            10
#define TIMEOUT_SECONDS      2

int g_tries = 0;
bool g_timeout = false;
crc crcTable[256];


int main(void)
{

    // Timeout handler
    struct sigaction my_timeout;

    my_timeout.sa_handler = timeout_alarm;
    if (sigfillset(&my_timeout.sa_mask) < 0){
        fprintf(stderr, "sigfillset failed\n");
        return 1;
    }
    my_timeout.sa_flags = 0;

    if (sigaction (SIGALRM, &my_timeout, 0) < 0) {
        fprintf(stderr, "sigaction() failed.\n");
        return 1;
    }


    // Initialize fastCRC
    crcInit();

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo *peer_address;
    if (getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
                address_buffer, sizeof(address_buffer),
                service_buffer, sizeof(service_buffer),
                NI_NUMERICHOST | NI_NUMERICSERV);
    printf("%s %s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Connecting...\n");
    if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }



    printf("Connected.\n");

    // TODO: If custom message / data needed...

    printf("Ready to send data to server\n");


    // GBN Client begins

    int packet_received = 0;
    int packet_sent = 0;
    uint8_t next_seq_num = 1;
    int window_size = 5;       // TODO: Needs to be received command line argumets
    int base = 1;
    char recv_packet[4096];
    int n_packets = strlen("Hello World");
    
    do { 

    

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
        FD_SET(0, &reads);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        if(select(socket_peer+1, &reads, 0,0, &timeout) < 0) {
            if (errno == EINTR) {
                printf("TImeout! %d more tries...\n", MAXTRIES - g_tries);
                continue;
            }
            else {
                fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
                break;

            }
            
        }
        
        if(FD_ISSET(socket_peer, &reads)) {
            printf("----- Packet Receive Start -------\n");
            memset(&recv_packet, 0, sizeof(recv_packet));
            int bytes_received = recv(socket_peer, recv_packet, 4096, 0);
            if (bytes_received < 1 ) {
                printf("Connection close by peer\n");
                break;
            }
            //printf("Received (%d bytes): %.*s\n", bytes_received, (int)bytes_received, recv_packet);

            // TODO: CRC check of data

            char *data = malloc(sizeof(char) * bytes_received + 1);
            for (long i = 0; i < bytes_received; i++) {
                data[i] = recv_packet[i];
            }
            data[bytes_received] = '\0';
            //printf("Received data: %x%s\n", data[0], &data[1]);

            crc crc_result = crcFast((crc *)data, bytes_received);

            if (crc_result == 0) {
                printf("Packet ok\n");
                base = recv_packet[0];
                printf("Base: %d\n", base);
                base++;     
                packet_received++;
                if (base == next_seq_num) {
                    alarm(0);
                } 
                else alarm(TIMEOUT_SECONDS);
                
            }
            else if (crc_result != 0) {
                printf("CRC error!\n");
            }
            printf("----- Packet Receive End -------\n\n");
           
            free(data);
            
            
        }

        // Send data
            if (next_seq_num < (base + window_size)) {
                char *message = "Hello World";
                
                
                char seq_message[4096];
                snprintf(seq_message, sizeof seq_message, "%c%c", next_seq_num, message[next_seq_num - 1]); 

                
                crc *data_to_send = malloc(sizeof(char) * 2);
                int len = strlen(seq_message);


                for (long i = 0; i < len; i++) {
                    data_to_send[i] = seq_message[i];
                }
                data_to_send[len] = '\0';
                
                // TODO: CRC needs to take account the sequence number
                
                crc crc_send = crcFast(data_to_send, len);

                char *packet = malloc(sizeof(char) * 100);
                int size = make_packet(next_seq_num, message[next_seq_num - 1], crc_send, packet);
                
                printf("----- Sending Packet %d -------\n", next_seq_num); 
                //printf("Data: %s\n", packet);
                
                int bytes_sent = send(socket_peer, packet, size, 0);

                if (base == next_seq_num) {
                    alarm(TIMEOUT_SECONDS);
                }
                if (bytes_sent < 1) {
                    fprintf(stderr, "Error occurred\n");
                    break;
                }
                // free(outgoing_data);
                printf("Sent %d bytes. Data: %c\n", bytes_sent, packet[1]);
                free(data_to_send);
                free (packet);
                next_seq_num++;
                packet_sent++;
                printf("----- Packet Send End -------\n\n"); 
                
            }
            if (g_timeout == true) {
                next_seq_num = base;
                packet_received = next_seq_num;
                g_timeout = false;
                printf("Base = %d, Next Seq: %d\n", base, next_seq_num);
                alarm(TIMEOUT_SECONDS);

            }
    }  while ((base < n_packets + 1) && g_tries < MAXTRIES);

    // Teardown sending SEQ 0 Data 0 with 0x69
    printf("------- Teardown the connection -------\n\n");
    char teardown[5];
    uint8_t teardown_seq = 0;
    char *teardown_data = "00";
    crc teardown_crc = 0x90;
    int size = make_packet(teardown_seq, *teardown_data, teardown_crc, teardown);
    send(socket_peer, teardown, size, 0);

    freeaddrinfo(peer_address);
    printf("Tries: %d \t Packets sent: %d \t Packets received: %d\n", g_tries, packet_sent, packet_received);
    CLOSESOCKET(socket_peer);

    printf("Finished\n\n");

    return 0;
}


void timeout_alarm(__attribute__((unused)) int ignore)
{
    g_tries++;
    g_timeout = true;

}

int make_packet (uint8_t next_sequence, char data, crc packet_crc, char *packet)
{

    if (next_sequence < 0) {
        return -1;
    }

    size_t len = snprintf(NULL, 0, "%c%c%c", next_sequence, data, packet_crc);

    snprintf(packet, len + 1,  "%c%c%c", next_sequence, data, packet_crc);

    return len;
}
