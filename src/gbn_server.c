/******************************************
 * 
 * Filename:    gbn_server.c
 * 
 * Description: udp_server for university exercise.
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
#include <ctype.h>

// Networking Headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

// Local Headers
#include "../include/crc.h"

#define ISVALIDSOCKET(s)    ((s) >= 0)
#define CLOSESOCKET(s)      close(s)
#define SOCKET              int
#define GETSOCKETERRNO()    (errno)

#define DEFAULT_PORT        "6666"

typedef struct gbn_packet
{
    int seq_no;
    char payload[20];
    uint8_t crc;
} gbp_packet;

crc crcTable[256];

SOCKET configure_socket(struct addrinfo *bind_address);

int main(int argc, char *argv[])
{
    char *port = NULL;
    port = DEFAULT_PORT;
    if (argc < 2) {
        port = argv[1];
    }

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
    freeaddrinfo(bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = configure_socket(bind_address);

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    printf("Waiting for connections...\n");

    while(1) {
        fd_set reads;
        reads = master;
        if(select(max_socket + 1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        } // if

        // Connection established
        if (FD_ISSET(socket_listen, &reads)) {
            struct sockaddr_storage client_address;
            socklen_t client_len = sizeof(client_address);

            char read[1024];
            long bytes_received = recvfrom(socket_listen, read, 1024, 0, (struct sockaddr *) &client_address, &client_len);
            if (bytes_received < 1) {
                fprintf(stderr, "connection closed. (%d)\n", GETSOCKETERRNO());
                return 1;   // or continue???
            } 

            // Received
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

            /* What do with the packet */


            // Send something back
            sendto(socket_listen, read, sizeof(read), 0, (struct sockaddr *) &client_address, client_len);
            memset(read, 0, sizeof(read));
        }
    } // While

    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

    printf("Finished.\n");

    return 0;
}


SOCKET configure_socket(struct addrinfo *bind_address)
{


    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    printf("Starting....\n");
    if(!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket failed. (%d)\n", GETSOCKETERRNO());
        // TODO: What to return as an error?
        return -1;
    }

    printf("Binding socket to local address...\n");
    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return -1;
    }
    
    return socket_listen;
}
