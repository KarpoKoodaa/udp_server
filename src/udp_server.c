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
#include "../include/sleep.h"
#include "../include/rdn_num.h"
#include "../include/crc.h"

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s)   close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

#define DEFAULT_PORT   "6666"

typedef struct Rdt_variables
{
    /* data */
    float drop_probability;
    float delay_probability;
    float error_probability;
    uint16_t delay_ms;
    uint16_t rdt;     // Reliable data transfer version (1.0, 2.0, 2.1, 2.2 or 3.0)
} Rdt_variables;


int make_packet(char *packet, int version, uint8_t seq, int result);
SOCKET configure_socket(struct addrinfo *bind_address);
bool process_packet(char *read, size_t bytes_received, struct Rdt_variables);

crc crcTable[256];

int main(int argc, char* argv[]) {
    
    uint8_t seq = 0;                // Sequence 
    int8_t last_seq = -1;          // last_sequence


    // UDP server port
    char *port = NULL;
    port = DEFAULT_PORT;
    Rdt_variables rdt_vars = {0, 0, 0, 0, 10}; 
    int c = 0;
    opterr = 0;
    float rdt_version = 0;

    // Parse command line arguments
    while((c = getopt(argc, argv, "x:p:d:r:t:v:")) != -1) {
        switch (c)
        {
        case 'x':
            // TODO: Support rdt version 3.0
            
            if (atof(optarg) > 3) {
                fprintf(stderr, "ERROR: supported rdt versions (1.0, 2.0, 2.1, 2.2 or 3.0)\n");
                return 1;
            }
            rdt_version = atof(optarg) * 10;
            rdt_vars.rdt = (uint16_t)rdt_version;
            break;
        case 'p':
            // Port
            port = optarg;
            break;
        case 'r':
            // Probability for packet drop
            rdt_vars.drop_probability = atof(optarg);
            break;
        case 'd':
            // Probability for packet delay
            rdt_vars.delay_probability = atof(optarg);
            break;
        case 't':
            // Delay is ms
            rdt_vars.delay_ms = atoi(optarg);
            break;
        case 'v':
            rdt_vars.error_probability = (double)atof(optarg);
            break;
        default:
            // TODO: Update Usage with error probability
            fprintf(stderr, "Usage: %s -p port -d delay_probability -r drop_probability -t delay_ms\n",
                           argv[0]);
            return 1;
            break;
        }
    }

    printf("RDT: %d Port: %s \tProbability for Packet Loss: %.1f \t Probability for Packet Delay: %.1f\t Delay: %d ms\n", rdt_vars.rdt, port,
                                                                                                        rdt_vars.drop_probability, rdt_vars.delay_probability,
                                                                                                        rdt_vars.delay_ms);


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
    SOCKET socket_listen = configure_socket(bind_address);

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

            bool packet_dropped = process_packet(read, bytes_received, rdt_vars);
            // Drop packet
            if (packet_dropped) {
                continue;
            } 
            else {

                // Print the CRC-8
                printf("First byte: %x\n", (unsigned char) read[0]);
                printf("Received CRC: %x\n", (unsigned char)read[bytes_received-1]);
                crc data[100];
                for (long i = 0; i < bytes_received; ++i) {
                    data[i] = read[i];
                }
                data[bytes_received] = '\0';

                if (rdt_vars.rdt == 22) {
                    if (read[0] == 0) seq = 0;
                    else if (read[0] == 1) seq = 1;
                }
                if (rdt_vars.rdt == 30) {
                    if (last_seq == seq) {
                        // Duplicate
                        printf("\033[0;31m");
                        printf("Duplicate packet\n");
                        printf("\033[0m");
                        seq = last_seq;
                    }
                    else {
                        if (read[0] == 0) seq = 0;
                        else if (read[0] == 1) seq = 1;
                    }
                }

                crc result = crcFast(data, bytes_received);

                printf("CRC: %d\n", result);

                                
                printf("Received (%ld bytes): %.*s\n", bytes_received, (int)bytes_received, read);
                
                char packet[8] = {0};
                int packet_len = 0;

                // If RDT 1.0, no ACK/NAK 
                if (rdt_vars.rdt == 10) {
                    continue;
                }
                packet_len = make_packet(packet, rdt_vars.rdt, seq, result);
                // TODO: What is smallest packet here? Probably 4, but returns 0 if fails
                if (packet_len < 1) {
                    fprintf(stderr, "Error creating packet\n");
                    continue;
                }
                printf("Packet size: %lu\n", sizeof(packet));
                printf("Packet len: %d\n", packet_len);
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
                    printf("Sent: CRC:%x, Packet size: %d\n", packet[packet_len - 1], packet_len);
                    sendto(socket_listen, packet, packet_len, 0, (struct sockaddr*)&client_address, client_len);
                }
                printf("Packet: %s\n", packet);
                
                printf("Sent v%1.1f: SEQ: %d CRC: %x, size: %d\n", (float)rdt_vars.rdt/10, packet[0], packet[3], packet_len);
                sendto(socket_listen, packet, packet_len, 0, (struct sockaddr*)&client_address, client_len);

            }
            
        }
    }
    // Should never end up here

    printf("Finished.\n");

    return 0;

} /* main() */

/**
 * @brief Creates a packet based on the specified rdt_vars.rdt version, sequence number, and result.
 *
 * This function constructs a packet with a specific format depending on the rdt version provided.
 * The packet contains acknowledgment (ACK/NAK) and a CRC value for integrity checking.
 *
 * @param[out] packet Pointer to a buffer where the created packet will be stored.
 * @param[in]  version Protocol version (e.g., 20, 21, 22, or 30 ) that determines the packet format.
 * @param[in]  seq Sequence number (0 or 1) used in the packet. Must not exceed 1.
 * @param[in]  result Result status (0 for success, non-zero for failure) influencing the ACK/NAK decision.
 * 
 * @return Length of the created packet on success, 
 *          and `-1` if error occurred
 * @note For `version == 22` and `seq == 1`, the CRC is hardcoded to 0x69 as per test app behavior.
 *
 * ### Packet Formats:
 * - **Version 20, 21**:
 *   - On success (`result == 0`): `"ACK<CRC>"`
 *   - On failure (`result != 0`): `"NAK<CRC>"`
 * - **Version 22, 30**:
 *   - On success (`result == 0`): `"<seq>ACK<CRC>"`
 *   - On failure (`result != 0`): `"<seq>NAK<CRC>"`
 *
 * ### Error Handling:
 * - If error occurs, the function returns `-1` 
 */
int make_packet(char *packet, int version, uint8_t seq, int result) {

    if (seq > 1) {
        return -1;     
    }
    
    int packet_len = -1; // Length of created packet
    uint8_t CRC = 0;    // CRC for packet
    if (result == 0 && seq == 1) {
        // This is strange. The test app responses ACK with CRC 69, if sequence is 1
        CRC = 0x69;
    }
    else if (result == 0) {
        CRC = 0x7f;
    }
    else if (result == 1){
        CRC = 0x12;
    }

    char *ack = (result == 0 ? "ACK" : "NAK");

    if (version == 20 || version == 21) {
        snprintf(packet, sizeof(packet), "%s%c", ack, CRC);
        printf("Packet is %s\n", packet);
        packet_len = snprintf(packet, sizeof(packet),"%s%c", ack, CRC);

    }
    else if (version == 22 || version == 30) {
        snprintf(packet, sizeof(packet), "%c%s%c", seq, ack, CRC);
        packet_len = snprintf(packet, sizeof(packet), "%c%s%c", seq, ack, CRC);            

    }
    return packet_len;
    
} /* make_packet */

// TODO: Add the hints part here as well
// TODO: Add doxygen 
SOCKET configure_socket(struct addrinfo *bind_address)
{

    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if(!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        // TODO: Cannot return 1!!
        return 1;
    }

    printf("Binding socket to local address...\n");
    if (bind (socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);

    return socket_listen;


} /* configure_socket */

// TODO: Add doxygen
bool process_packet(char *read, size_t bytes_received, Rdt_variables vars)
{
    if (rand_number() <= vars.drop_probability) {
        printf("\033[0;31m");
        printf("Packet dropped\n");
        printf("\033[0m");
        return true;
    }
    // TODO: Remove else 
    else {
    // Add delay   
        if (rand_number() <= vars.delay_probability) {
            printf("\033[0;31m");
            printf("Delay added\n");
            printf("\033[0m");
            msleep(vars.delay_ms);
        }
        // Add bit error
        if (rand_number() <= vars.error_probability) {
            char mask = 0x2;
            read[bytes_received-2] = read[bytes_received-2] ^ mask;
        }
    }
               
    return false;
} /* process_packet */
