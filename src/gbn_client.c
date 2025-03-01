 /**
 * 
 * Filename:    gbn_client.c
 * 
 * Description: Go-Back-N UDP client written in C. This implements a reliable data transfer
 *              mechanism over an unreliable UDP connection by handling packet loss, retransmissions, and acknowledgments. 
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
size_t make_packet (uint8_t next_sequence, char data, crc packet_crc, char *packet);

#define RED     "\033[1;31m"
#define ORANGE  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define RESET   "\033[0m"

#define ISVALIDSOCKET(s)    ((s) >= 0)
#define CLOSESOCKET(s)      close(s)
#define SOCKET              int
#define GETSOCKETERRNO()    (errno)

#define SERVER_IP           "127.0.0.1"
#define DEFAULT_PORT        "6666"
#define MAXTRIES            10
#define TIMEOUT_SECONDS      2
#define MESSAGE             "Hello World from GB-N"

enum CRC_Status {
    OK,
    NOK,
};


volatile int g_tries = 0;
volatile bool g_timeout = false;
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
    if (getaddrinfo(SERVER_IP, DEFAULT_PORT, &hints, &peer_address)) {
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

    printf("Ready to send data to server\n");

    // GBN Client begins

    size_t packet_received = 0;
    size_t packet_sent = 0;
    uint8_t next_seq_num = 1;
    int window_size = 5;       // TODO: Needs to be received command line argumets
    size_t base = 1;
    char recv_packet[4096];
    size_t n_packets = strlen(MESSAGE);
    
    do { 

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
        FD_SET(0, &reads);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        if(select(socket_peer+1, &reads, 0,0, &timeout) < 0) {
            // Timeout occurred
            if (errno == EINTR) {
                printf("TImeout! %d more tries...\n", MAXTRIES - g_tries);
                printf(BLUE "----- Timeout occurred -------\n" RESET);
                printf("Retries left: %d | Next SEQ: %d\n", MAXTRIES - g_tries, next_seq_num);
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


            // Reserving memory for incoming message. Byte_received misses the NULL to terminate the string
            char *data = malloc(bytes_received + 1);
            if (!data) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
            }
            memcpy(data, recv_packet, bytes_received);
            data[bytes_received] = '\0';

            // Check if the packet is corrupted or not
            crc crc_result = crcFast((crc *)data, bytes_received);
            free(data);

            if (crc_result == OK) {
                base = recv_packet[0];
                printf("ACK received: SEQ %zu | CRC Check: OK\n", base); 
                
                // Increase packet counters
                base++;     
                packet_received++;
                
                // If the base is same than next packet to send, zero the timer
                if (base == next_seq_num) {
                    alarm(0);
                } 
                // Otherwise initiate the timer
                else alarm(TIMEOUT_SECONDS);
                
            }
            else if (crc_result == NOK) {
                printf("ACK Received: SEQ %d | CRC Check: NOK\n", recv_packet[0]);
            }
            printf("----- Packet Receive End -------\n\n");
           
            
            
        }

        // Send data to Server if there is room in sending window
            if (next_seq_num < (base + window_size)) {
                char *message = MESSAGE;
                
                
                char seq_message[4096];
                snprintf(seq_message, sizeof seq_message, "%c%c", next_seq_num, message[next_seq_num - 1]); 

                // Event though the message size of is 2 chars, the below code is prepared for bigger messages.
                int len = strlen(seq_message);
                crc *data_to_send = malloc(len * sizeof(char));
                if (!data_to_send) {
                    fprintf(stderr, "Memory allocation failed");
                    exit(1);
                } 

                for (long i = 0; i < len; i++) {
                    data_to_send[i] = seq_message[i];
                }
                data_to_send[len] = '\0';
                
                crc crc_send = crcFast(data_to_send, len);
                free(data_to_send);

                char *packet = malloc(sizeof(char) * len);
                if (!packet) {
                    fprintf(stderr, "Memory allocation failed\n");
                    exit(1);

                }
                size_t size = make_packet(next_seq_num, message[next_seq_num - 1], crc_send, packet);
                
                printf("----- Sending Packet %d -------\n", next_seq_num); 
                
                int bytes_sent = send(socket_peer, packet, size, 0);

                // Start timer
                if (base == next_seq_num) {
                    alarm(TIMEOUT_SECONDS);
                }
                if (bytes_sent < 1) {
                    fprintf(stderr, "Error occurred\n");
                    break;
                }
                // free(outgoing_data);
                printf("Packet sent: SEQ %d | Data: %c | Bytes: %d\n", packet[0], packet[1], bytes_sent);
                
                // Cleaning up memory
                free (packet);

                // Increase packet counters
                next_seq_num++;
                packet_sent++;

                printf("----- Packet Send End -------\n\n"); 
                
            }
            if (g_timeout == true) {
                next_seq_num = base;
                packet_received = next_seq_num;
                g_timeout = false;
                printf(BLUE "----- Timeout occurred -------\n" RESET);
                printf("Window base: %zu | Next SEQ: %d\n", base, next_seq_num);
                alarm(TIMEOUT_SECONDS);
                printf(BLUE "----- Timeout end -------\n\n" RESET);

            }
    }  while ((base < n_packets + 1) && g_tries < MAXTRIES);

    // Teardown sending SEQ 0 Data 0 with 0x69
    printf("------- ALL PACKETS SENT AND RECEIVED -------\n");
    printf("------- Teardown the connection -------\n\n");
    char teardown[5];
    uint8_t teardown_seq = 0;
    char *teardown_data = "00";
    crc teardown_crc = 0x90;
    int size = make_packet(teardown_seq, *teardown_data, teardown_crc, teardown);
    send(socket_peer, teardown, size, 0);

    freeaddrinfo(peer_address);
    printf("Retries left: %d \t Packets sent: %zu \t Packets received: %zu\n", g_tries, packet_sent, packet_received);
    CLOSESOCKET(socket_peer);

    printf("Finished\n\n");

    return 0;
}


void timeout_alarm(__attribute__((unused)) int ignore)
{
    g_tries++;
    g_timeout = true;

}

/**
 * @brief Constructs a data packet with a sequence number, data, and CRC checksum.
 *
 * This function creates a packet by formatting the sequence number, data byte, 
 * and CRC checksum into a string format.
 *
 * @param next_sequence The sequence number of the packet.
 * @param data The data character to be sent.
 * @param packet_crc The CRC checksum for error detection.
 * @param packet A pointer to the buffer where the constructed packet will be stored.
 * 
 * @return The size of the created packet, or -1 if the sequence number is invalid.
 */
size_t make_packet (uint8_t next_sequence, char data, crc packet_crc, char *packet)
{

    size_t len = snprintf(NULL, 0, "%c%c%c", next_sequence, data, packet_crc);

    snprintf(packet, len + 1,  "%c%c%c", next_sequence, data, packet_crc);

    return len;
}
