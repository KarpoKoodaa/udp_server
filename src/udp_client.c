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
int make_packet (int next_sequence, char *data, crc packet_crc);

#define ISVALIDSOCKET(s)    ((s) >= 0)
#define CLOSESOCKET(s)      close(s)
#define SOCKET              int
#define GETSOCKETERRNO()    (errno)

#define DEFAULT_PORT        "6666"
#define MAXTRIES            5
#define TIMEOUT_SECONDS      5

int g_tries = 0;
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
    int next_seq_num = 1;
    int window_size = 5;       // TODO: Needs to be received command line argumets
    int base = 1;
    // char *packet[window_size];            // Most likely needs to be struct or char **
    char recv_packet[4096];

    do {
    

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
        FD_SET(0, &reads);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        if(select(socket_peer+1, &reads, 0,0, &timeout) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            break;
        }

        if(FD_ISSET(socket_peer, &reads)) {
            memset(&recv_packet, 0, sizeof(recv_packet));
            int bytes_received = recv(socket_peer, recv_packet, 4096, 0);
            if (bytes_received < 1 ) {
                printf("Connection close by peer\n");
                break;
            }
            printf("Received (%d bytes): %.*s\n", bytes_received, (int)bytes_received, recv_packet);

            // TODO: CRC check of data
            crc *data = malloc(sizeof(char) * bytes_received + 1);
            for (long i = 0; i < bytes_received; i++) {
                data[i] = recv_packet[i];
            }
            data[bytes_received] = '\0';
            printf("Received data: %x %s\n", data[0], &data[1]);
            crc crc_result = crcFast(data, bytes_received);

            if (crc_result == 0) {
                printf("Packet ok\n");
                base = recv_packet[0];
                base++;
                if (base == next_seq_num) alarm(0);
                else alarm(TIMEOUT_SECONDS);
                packet_received++;
            }
            else if (crc_result != 0) {
                printf("Packet error!\n");
            }
            free(data);
            
        }

        // Send data
        //if (FD_ISSET(0, &reads)) {
            if (next_seq_num < (base + window_size)) {
                // int bytes_outgoing = 13;
                char *message = "Hello World%";
                // crc *outgoing_data = malloc(sizeof(char)* bytes_outgoing + 1);
                // crc packet_crc = crcFast(outgoing_data, bytes_outgoing);
                // crc packet_crc = crcFast(message, bytes_outgoing);
                
                // size_t size = make_packet(next_seq_num, packet[next_seq_num], packet_crc);

                // send or sendto
                printf("Sending data.....\n");
                // int bytes_sent = sendto(socket_peer, message, strlen(message), 0, peer_address->ai_addr, peer_address->ai_addrlen);
                int bytes_sent = send(socket_peer, message, strlen(message), 0);
                if (base == next_seq_num) {
                    alarm(TIMEOUT_SECONDS);
                }
                if (bytes_sent < 1) {
                    fprintf(stderr, "Error occurred\n");
                    break;
                }
                // free(outgoing_data);
                next_seq_num++;
                packet_sent++;
            }
        //}

        
        // Receive da
        // else {
        //     // Wait until more window is open
        //     // To be considered if something needed here...
        // }
    } while (packet_received < packet_sent && g_tries < MAXTRIES);

    // const char *message = "Hello World";
    
    // printf("Sending: %s\n", message);
    // int bytes_sent = sendto(socket_peer, message, strlen(message), 0, peer_address->ai_addr, peer_address->ai_addrlen);
    // printf("Sent %d bytes\n", bytes_sent);
    freeaddrinfo(peer_address);
    printf("Tries: %d Packets sent: %d Packets received: %d next seq num: %d\n", g_tries, packet_sent, packet_received, next_seq_num);
    CLOSESOCKET(socket_peer);

    printf("Finished\n");

    return 0;
}


void timeout_alarm(__attribute__((unused)) int ignore)
{
    g_tries++;

}

int make_packet (int next_sequence, char *data, crc packet_crc)
{

    if (next_sequence < 0) {
        return -1;
    }

    size_t len = snprintf(NULL, 0, "%s%x", data, packet_crc);

    return len;
}
