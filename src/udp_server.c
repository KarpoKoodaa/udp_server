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

crc crcTable[256];

int main(int argc, char* argv[]) {
    
    // Probability variables for packet drops and delays
    // TODO: Consider adding these to cli parameters
    double packet_drop = 0.0;
    double packet_delay = 0.0;
    int delay_ms = 0;

    // UDP server port
    char *port = NULL;
    port = "6666";
    
    int c = 0;
    opterr = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    while((c = getopt(argc, argv, "p:l:d:t:")) != -1) {
        switch (c)
        {
        case 'p':
            port = optarg;
            break;
        case 'l':
            packet_drop = (double)atof(optarg);
            break;
        case 'd':
            packet_delay = (double)atof(optarg);
            break;
        case 't':
            delay_ms = atoi(optarg);
            break;
        case '?':
            fprintf(stderr, "Usage: %s [-p port] [-l packet loss probability] [-d packet delay probability] [-t delay in ms]\n", argv[0]);
            return 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-p port] [-l packet_loss probability]\n",
                           argv[0]);
            return 1;
            break;
        }
    }

    printf("port: %s \tPacket Loss: %.1f \t Packet Delay: %.1f\t Delay: %d ms\n", port, packet_drop, packet_delay, delay_ms);


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
            if (rand_number() <= packet_drop) {
                printf("Packet dropped\n");
            }
            else {
                // Add delay   
                if (rand_number() <= packet_delay) {
                    printf("Delay added\n");
                    msleep(delay_ms);
                }
                // Add bit error
                else if (rand_number() <= packet_delay) {
                    char mask = 0x2;
                    read[bytes_received-2] = read[bytes_received-2] ^ mask;
                }
                char mask = 0x2;
                read[bytes_received-2] = read[bytes_received-2] ^ mask;

                // Print the CRC-8
                printf("%x\n", (unsigned char)read[bytes_received-1]);
                crc data[8];
                for (long i = 0; i < bytes_received; ++i) {
                    data[i] = read[i];
                }
                data[bytes_received] = '\0';

                printf("Data: %s\n", data);
                crc result = crcFast(data, bytes_received);

                printf("CRC: %d\n", result);

                                
                printf("Received (%ld bytes): %.*s\n", bytes_received, (int)bytes_received, read);
            
                printf("Remote address is: ");
                char address_buffer[100];
                char service_buffer[100];
                getnameinfo(((struct sockaddr*)&client_address),
                        client_len,
                        address_buffer, sizeof(address_buffer),
                        service_buffer, sizeof(service_buffer),
                        NI_NUMERICHOST | NI_NUMERICSERV);
                printf("%s %s\n", address_buffer, service_buffer);
            }
            
        }
    }
    // Should never end up here

    printf("Finished.\n");

    return 0;

} /* main() */
