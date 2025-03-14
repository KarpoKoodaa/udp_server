/**
 * 
 * Filename:    sr_client.c
 * 
 * Description: Selective Repeat UDP client written in C. This implements a reliable data transfer
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
#define MAXTRIES            20
#define TIMEOUT_SECONDS     2
#define WINDOW_SIZE         5 
#define MESSAGE             "Hello World from Selective Repeat"

enum Packet_ack {
    NACK,
    ACK,
};

volatile int g_tries = 0;
volatile bool g_timeout = false;
crc crcTable[256];

int packet_timer[WINDOW_SIZE];      // Timer for a sent packets. Tracking ony packets within window
int packet_tracker[WINDOW_SIZE];       // Tracks if a packet is sent


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

    // Configure remote address and create a socket
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

    // Selective Repeat Client begins

    size_t packet_received = 0;
    size_t packet_sent = 0;
    uint8_t next_seq_num = 1;
    int window_size = WINDOW_SIZE;       // TODO: Needs to be received command line argumets
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
            // Timeout occured. 
            if (errno == EINTR) {
                                
                alarm(TIMEOUT_SECONDS);
                continue;
            }
            else {
                fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
                break;

            }
            
        }
        
        // Receive data from Server
        if(FD_ISSET(socket_peer, &reads)) {

            printf("----- Packet Receive Start -------\n");
            memset(&recv_packet, 0, sizeof(recv_packet));
            int bytes_received = recv(socket_peer, recv_packet, 4096, 0);
            if (bytes_received < 1 ) {
                printf("Connection close by peer\n");
                break;
            }

            // Reserving memory for incoming message. Bytes_received is missing the NULL to terminate the string
            // char *data = malloc(sizeof(char) * bytes_received + 1);
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

            // If not corrupted
            if (crc_result == 0) {
                int rcv_seq = 0;
                
                rcv_seq = recv_packet[0];
                printf("ACK received: SEQ %d | CRC Check: OK\n", rcv_seq);

                packet_tracker[rcv_seq] = ACK;
                packet_timer[rcv_seq] = 0;
               
                if ((rcv_seq - base) < 2) {
                    size_t i = base;
                    while(i < base + WINDOW_SIZE && packet_tracker[i] == ACK) {
                        i++;
                    }
                    base = i;

                }
                packet_received++;
                
            }
            else if (crc_result != 0) {
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
                
                // Create a packet that is sent to server
                size_t size = make_packet(next_seq_num, message[next_seq_num - 1], crc_send, packet);
                
                printf("----- Sending Packet %d -------\n", next_seq_num); 
                
                int bytes_sent = send(socket_peer, packet, size, 0);

                packet_tracker[next_seq_num] = NACK;
                packet_timer[next_seq_num] = TIMEOUT_SECONDS;

                // Starting the timer
                if (base == next_seq_num) {
                    alarm(TIMEOUT_SECONDS);
                }
                if (bytes_sent < 1) {
                    fprintf(stderr, "Error occurred\n");
                    break;
                }

                printf("Packet sent: SEQ %d | Data: %c | Bytes: %d\n", packet[0], packet[1], bytes_sent);

                // Cleaning up memory
                free (packet);
                
                // Increasing packet counters
                next_seq_num++;
                packet_sent++;

                printf("----- Packet Send End -------\n\n"); 
                
            }
            // If the timeout occured
            if (g_timeout == true) {
                g_timeout = false;
                
                // Decreasing individual packet timers
                for (uint8_t i = base; i < base + WINDOW_SIZE; ++i) {
                    if (packet_timer[i] > 0) {
                        packet_timer[i]--;

                        // If packet have timeout and do ACK received, resending packets
                        if (packet_timer[i] == 0 && packet_tracker[i] == NACK) {
                            // TODO: Make and send packet
                            char *message = MESSAGE;

                            char seq_message[2048];
                            snprintf(seq_message, sizeof(seq_message), "%c%c", i, message[i-1]);
                            
                            int len = strlen(seq_message);
                            crc *data_to_resend = malloc(sizeof(char) * len);

                            for (uint8_t y = 0; y < len; ++y) {
                                data_to_resend[y] = seq_message[y];
                            }
                            data_to_resend[len] = '\0';

                            crc crc_send = crcFast(data_to_resend, len);

                            char *packet = malloc(sizeof(char) * len);
                            int size = make_packet(i, message[i-1], crc_send, packet);
                            printf(BLUE "----- Timeout occurred -------\n" RESET);
                            printf(BLUE "----- Resending Packet %d -------\n" RESET, i); 

                            int bytes_sent = send(socket_peer, packet, size, 0);

                            printf("Packet resent: SEQ %d | Data: %c | Bytes: %d\n", packet[0], packet[1], bytes_sent);
            
                            packet_tracker[i] = NACK;
                            packet_timer[i] = TIMEOUT_SECONDS;
                            if (bytes_sent < 1) {
                                fprintf(stderr, "Error occurred\n");
                                break;
                            }
            
                            free(data_to_resend);
                            free (packet);
                            printf(BLUE "----- Packet Resend End -------\n\n" RESET); 


                        }
                    }
                }
                alarm(TIMEOUT_SECONDS);
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


