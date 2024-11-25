/******************************************
 * 
 * Filename:    udp_server.c
 * 
 * Description: udp_server for university exercise.
 *              Based on Lewis Van Winkle's "Hands-on Network Programming with C" book
 * 
 * Copyright (c) 2024 Kariantti Laitala
 * Permission tba
 *******************************************/

// 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Getopt
#include <ctype.h>

// Network 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

// Locals
#include "../include/sleep.h"
#include "../include/rdn_num.h"
#include "../include/crc.h"

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s)   close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

#define DEFAULT_PORT   "6666"

int make_packet(char *packet, int version, uint8_t seq, int result);

crc crcTable[256];

int main(int argc, char* argv[]) {
    
    // Probability variables for packet drops and delays
    double drop_prob = 0.0;
    double delay_prob = 0.0;
    int delay_ms = 0;
    double error_probability = 0.1;
    unsigned int rdt = 0;     // Reliable data transfer version (1.0, 2.0, 2.1, 2.2 or 3.0)
    uint8_t seq = 0;                // Sequence

    // UDP server port
    char *port = NULL;
    port = DEFAULT_PORT;
    
    int c = 0;
    opterr = 0;
    float rdt_version = 0;

    // Parse command line arguments
    while((c = getopt(argc, argv, "x:p:d:r:t:v:")) != -1) {
        switch (c)
        {
        case 'x':
            // TODO: Support rdt version as well
            
            if (atof(optarg) > 3) {
                fprintf(stderr, "ERROR: supported rdt versions (1.0, 2.0, 2.1, 2.2 or 3.0)\n");
                return 1;
            }
            rdt_version = atof(optarg) * 10;
            rdt = (int)rdt_version;
            break;
        case 'p':
            port = optarg;
            break;
        case 'r':
            drop_prob = (double)atof(optarg);
            break;
        case 'd':
            delay_prob = (double)atof(optarg);
            break;
        case 't':
            delay_ms = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -p port -d delay_prob -r drop_prob -t delay_ms\n",
                           argv[0]);
            return 1;
            break;
        }
    }

    printf("RDT: %d Port: %s \tProbability for Packet Loss: %.1f \t Probability for Packet Delay: %.1f\t Delay: %d ms\n", rdt, port, drop_prob, delay_prob, delay_ms);


    // Precompute CRC8 table for fastCRC
    crcInit();
    
    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, port, &hints, &bind_address);  

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if(!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Binding socket to local address...\n");
    if (bind (socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    printf("Waiting for connections....\n");

    while (1) {
        fd_set reads;
        reads = master;
        if(select(max_socket +1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }
        // Connection established
        if (FD_ISSET(socket_listen, &reads)) {
            struct sockaddr_storage client_address;
            socklen_t client_len = sizeof(client_address);

            char read[1024];
            long bytes_received = recvfrom(socket_listen, read, 1024, 0, (struct sockaddr *)&client_address, &client_len);
            if (bytes_received < 1) {
                fprintf(stderr, "connection closed. (%d)\n", GETSOCKETERRNO());
                return 1;
            }

            /* VIRTUAL SOCKET BEGINS */

            // Drop packet
            if (rand_number() <= drop_prob) {
                printf("Packet dropped\n");
            }
            else {
                // Add delay   
                if (rand_number() <= delay_prob) {
                    printf("Delay added\n");
                    msleep(delay_ms);
                }
                // Add bit error
                if (rand_number() <= error_probability) {
                    char mask = 0x2;
                    read[bytes_received-2] = read[bytes_received-2] ^ mask;
                }
               
                // Print the CRC-8
                printf("First byte: %x\n", (unsigned char) read[0]);
                printf("Received CRC: %x\n", (unsigned char)read[bytes_received-1]);
                crc data[8];
                for (long i = 0; i < bytes_received; ++i) {
                    data[i] = read[i];
                }
                data[bytes_received] = '\0';

                if (rdt == 22) {
                    if (read[0] == 0) seq = 0;
                    else if (read[0] == 1) seq = 1;
            }

                crc result = crcFast(data, bytes_received);

                printf("CRC: %d\n", result);

                                
                printf("Received (%ld bytes): %.*s\n", bytes_received, (int)bytes_received, read);
                
                char packet[8] = {0};
                int packet_len = 0;
                packet_len = make_packet(packet, rdt, seq, result);
                // TODO: What is smallest packet here?
                if (packet_len < 1) {
                    fprintf(stderr, "Error creating packet\n");
                    //TODO: Cannot break here as program exits. Find a solution.
                    break;
                }
                printf("Packet size: %lu\n", sizeof(packet));
                printf("Remote address is: ");
                char address_buffer[100];
                char service_buffer[100];
                getnameinfo(((struct sockaddr*)&client_address),
                        client_len,
                        address_buffer, sizeof(address_buffer),
                        service_buffer, sizeof(service_buffer),
                        NI_NUMERICHOST | NI_NUMERICSERV);
                printf("%s %s\n", address_buffer, service_buffer);
                
                printf("Result is %d\n", result);
                if (result != 0) {
                    printf("Sending: %x, %ld\n", packet[3], sizeof(packet)-1);
                    sendto(socket_listen, packet, sizeof(packet)-1, 0, (struct sockaddr*)&client_address, client_len);
                }
                printf("Packet: %s\n", packet);
                
                
                if (rdt == 0 && !result)
                {
                    // TODO: fix using packet_len
                    printf("Sending %c, %ld\n", packet[3], sizeof(packet)-2);
                    sendto(socket_listen, packet, sizeof(packet)-1, 0, (struct sockaddr*)&client_address, client_len);
                }
                else if (rdt == 10 && result == 0) {
                    // TODO: fix using packet_len
                    printf("Sending v1.0: %c, %ld\n", packet[3], sizeof(packet)-2);
                    sendto(socket_listen, packet, sizeof(packet)-2, 0, (struct sockaddr*)&client_address, client_len);
                }
                else if ((rdt == 20 || rdt == 21) && result == 0) {
                    // TODO: fix using packet_len
                    printf("Sending v2.0 or v2.1: %s, %ld\n", packet, sizeof(packet)-1);
                    sendto(socket_listen, packet, sizeof(packet), 0, (struct sockaddr*)&client_address, client_len);
                }
                else if (rdt == 22 && result == 0) {
                    printf("Sending v2.2: len: %d %s\n", packet_len, packet);
                    sendto(socket_listen, packet, packet_len, 0, (struct sockaddr*)&client_address, client_len);
                }
                

            }
            
        }
    }
    // Should never end up here

    printf("Finished.\n");

    return 0;

} /* main() */


/* 
    RDT packets:
    RDT1.0 = no ACK/NAK // TODO: Handle in main 
    RDT2.0 = ACK/NAK + (CHECKSUM?)
    RDT2.1 = ACK/NAK+CHECKSUM
    RDT2.2 = ACK/NAK+SEQ+CHECKSUM

*/
int make_packet(char *packet, int version, uint8_t seq, int result) {

    if (seq > 1) {
        return 1;     
    }

    int packet_len = 0;
    // Packet example: ACKCRC
    // TODO: Refactor these to similar to version 22
    // TODO: Fix to return check length of packet with snprintf
    if (version == 20 || version == 21) {
        if (!result) {
            strcpy(packet, "ACK");
            packet[3] = 0x7f; // CRC8
            packet[4] = '\0';
        }
        else {
            strcpy(packet, "NAK");
            packet[3] = 0x12;  // CRC8
            packet[4] = '\0';
        }
        return 0;
    }
    // Packet example: SEQ:ACK:CRC
    else if (version == 22) {
        if (!result) {
            uint8_t CRC = 0;

            // This is strange. The test app responses ACK with CRC 69, if sequency is 1
            if ( seq == 1){
                CRC = 0x69;
            } else CRC = 0x7f;

            snprintf(packet, sizeof(packet), "%cACK%c", seq, CRC);
            packet_len = snprintf(packet, sizeof(packet), "%cACK%c", seq, CRC);            
        }
        else {
            uint8_t CRC = 0x12;
            snprintf(packet, sizeof(packet), "%cNAK%c", seq, CRC);
            //TODO: Fix to return check length of packet with snprintf
        }
        return packet_len;
    }
       

    return 1;


}
