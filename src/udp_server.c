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
#include "../include/rdt.h"
#include "../include/gbn.h"
#include "../include/sr.h"

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s)   close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

#define WINDOW_SIZE     5
#define MAX_BUFFER_SIZE 50

#define DEFAULT_PORT   "6666"

#define RED     "\033[1;31m"
#define ORANGE  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define RESET   "\033[0m"


SOCKET configure_socket(struct addrinfo *bind_address);

crc crcTable[256];

int main(int argc, char* argv[]) {
    
    

    // UDP server port
    char *port = NULL;
    port = DEFAULT_PORT;
    bool rdt = true;
    Rdt_variables rdt_vars = {0, 0, 0, 0, 0, -1, 10}; 
    int c = 0;
    opterr = 0;
    float rdt_version = 0;

    float drop_probability = 0;

    

    bool gbn = false;
    
    int expected_seq_num = 1;

    bool sr = false;
    int rcv_base = 1;
    sr_receive_buffer_t sr_receive_buffer = {{false}, {0}};
    int last_seq = 0;


    // Parse command line arguments
    while((c = getopt(argc, argv, "x:p:d:r:t:v:gs")) != -1) {
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
            drop_probability = atof(optarg);
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
        case 'g':
            gbn = true;
            rdt = false;
            break;
        case 's':
            sr = true;
            rdt = false;
            break;
        default:
            // TODO: Update Usage with error probability
            fprintf(stderr, "Usage: %s -p port -d delay_probability -r drop_probability -t delay_ms\n",
                           argv[0]);
            return 1;
            break;
        }
    }

    if (rdt == true) {
        printf("RDT: %d Port: %s \tProbability for Packet Loss: %.1f \t Probability for Packet Delay: %.1f\t Delay: %d ms\n", rdt_vars.rdt, port,
                                                                                                        rdt_vars.drop_probability, rdt_vars.delay_probability,
                                                                                                        rdt_vars.delay_ms);
    }
    else if (gbn == true) {
        printf("GBN Port: %s \tProbability for Packet Loss: %.1f\n", port, drop_probability);
    }
    else if (sr == true) {
        port = DEFAULT_PORT;
        printf("Selective Repeat Port: %s \tProbability for Packet Loss %.1f\n", port, drop_probability);
    }
    // Precompute CRC8 table for fastCRC
    crcInit();
    
    char all_received[4096];
    
    // Preparing the Teardown data that is used to Teardown the connection. 
    char teardown[5];
    char *tear_data = "00";
    snprintf(teardown, 3, "%c%s%c", 0, tear_data, 0x90);
    
    
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
            memset(read, '\0' , sizeof(read));

            long bytes_received = recvfrom(socket_listen, read, 1024, 0, (struct sockaddr *)&client_address, &client_len);
            if (bytes_received < 1) {
                fprintf(stderr, "connection closed. (%d)\n", GETSOCKETERRNO());
                return 1;
            }

            /* VIRTUAL SOCKET BEGINS */
            if (rdt == true) {
                crc result = process_packet (read, bytes_received, &rdt_vars);

        
                printf("CRC: %d\n", result);

                                
                printf("Received (%ld bytes): %.*s\n", bytes_received, (int)bytes_received, read);
                
                char packet[8];
                memset(packet, 0, sizeof(packet));
                int packet_len = 0;

                // If RDT 1.0, no ACK/NAK 
                if (rdt_vars.rdt == 10) {
                    printf("rdt version %d\n", rdt_vars.rdt);
                    continue;
                }

                printf("SEQ: %d\n", rdt_vars.seq);

                    packet_len = make_packet(packet, rdt_vars.rdt, rdt_vars.seq, result);
                // TODO: What is smallest packet here? Probably 4, but returns 0 if fails
                if (packet_len < 1) {
                fprintf(stderr, "Error creating packet!\n");
                    continue;
                }
                
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
                    sendto(socket_listen, packet, strlen(packet), 0, (struct sockaddr*)&client_address, client_len);
                }
                printf("Packet: %s\n", packet);
                
                printf("Sent v%1.1f: SEQ: %d CRC: %x, size: %d\n", (float)rdt_vars.rdt/10, packet[0], packet[3], packet_len);
                sendto(socket_listen, packet, packet_len , 0, (struct sockaddr*)&client_address, client_len);

            }

            if ((rand_number() <= drop_probability) && (strncmp(teardown,read, 3) != 0)) {
                printf(RED "------- Packet Dropped -------\n\n" RESET);
                    
            }
            else if (gbn == true) {

                // printf("\nReceived (%ld bytes) Data: %c\n", bytes_received, read[1]);

                // // Not dropping packet if teardown received
                // if ((rand_number() <= drop_probability) && (strncmp(teardown,read, 3) != 0)) {
                //     printf("\033[0;31m");
                //     printf("------- Packet Dropped -------\n\n");
                //     printf("\033[0m");
                    
                // }
                // else {
                    if (strncmp(teardown, read, 3) == 0) {
                        printf("\n------- Teardown received -------\n\n");
                        break;
                    }
                    int gbn_result = gbn_process_packet(read, bytes_received, expected_seq_num);

                    
                    if (gbn_result == CRC_NOK) {
                        printf("Packet corrupted!\n");
                        continue;
                    } else if (gbn_result == SEQ_NOK) {
                        --expected_seq_num;
                        
                    } else all_received[expected_seq_num-1] = read[1];

                    printf("\n----- Sending Response -------\n");
                    char gbn_packet[10] = {0};
                    int packet_len = 0;

                    packet_len = gbn_make_packet(gbn_packet, expected_seq_num);
                    if (packet_len == -1) {
                        fprintf(stderr, "ERROR: Create packet failed");
                        continue;
                    }

                    printf("Remote address is: ");
                    char address_buffer[100];
                    char service_buffer[100];
                    getnameinfo(((struct sockaddr*)&client_address),
                        client_len,
                        address_buffer, sizeof(address_buffer),
                        service_buffer, sizeof(service_buffer),
                        NI_NUMERICHOST | NI_NUMERICSERV);
                    printf("%s %s\n", address_buffer, service_buffer);

                    printf("Sending response: %d%s\n",gbn_packet[0], &gbn_packet[1]); 
                    sendto(socket_listen, gbn_packet, packet_len, 0, (struct sockaddr*) &client_address, client_len);
                    expected_seq_num++;
                // }

            }

            /* Selective Repeat Server Part */
            else if (sr == true) {

                // Check if connection teardown is received
                if (strncmp(teardown, read, 3) == 0) {
                    printf("\n------- Teardown received -------\n\n");
                    break;
                }
                int sr_result = sr_process_packet(read, bytes_received);

                // If the Packet is corrupted
                if (sr_result == NAK) {
                    printf(RED "Packet Received | CRC Check: NOK\n\n" RESET);
                    continue;
                }
                
                char sr_packet[10] = {0};
                int packet_len = 0;
                
                // Checking that the Received packet is within the Receiving Window
                if (sr_result >= rcv_base && sr_result < rcv_base + WINDOW_SIZE) {
                    
                    if(sr_receive_buffer.received[sr_result] == false) {
                        sr_receive_buffer.received[sr_result] = true;
                        sr_receive_buffer.data[sr_result] = read[1];

                        if (sr_result == rcv_base) {
                            rcv_base = deliver_data(sr_receive_buffer, all_received, rcv_base);
                            
                        }
                    }
                    packet_len = sr_make_packet(sr_packet, sr_result);
                } 
                else if (sr_result >= rcv_base - WINDOW_SIZE && sr_result < rcv_base) {

                    // Packet is already received, but sending ACK anyway
                    packet_len = sr_make_packet(sr_packet, sr_result);
                }
                else {
                    // Packet out of range, ignoring
                    printf("Packet %d out of range, ignore\n", sr_result);
                    printf("Current rcvbase: %d\n", rcv_base);
                    continue;
                }

                printf("\n----- Sending Response -------\n");

                if (packet_len == -1) {
                    fprintf(stderr, "ERROR: Create packet failed");
                    continue;
                }

                printf("Remote address is: ");
                char address_buffer[100];
                char service_buffer[100];
                getnameinfo(((struct sockaddr*)&client_address),
                    client_len,
                    address_buffer, sizeof(address_buffer),
                    service_buffer, sizeof(service_buffer),
                    NI_NUMERICHOST | NI_NUMERICSERV);
                printf("%s %s\n", address_buffer, service_buffer);

                printf("Sending response: %d%s\n",sr_packet[0], &sr_packet[1]); 
                sendto(socket_listen, sr_packet, packet_len, 0, (struct sockaddr*) &client_address, client_len);
                last_seq = sr_result;
                printf("----- Sending Response End -------\n\n");


            }
            

            
        }
    }
    // TODO: This might not work for GBN
    // Adding NULL to terminate the received data. Size of data depends on the mode (GBN or SR)
    if (gbn == true) {
        last_seq = expected_seq_num;

    }
    else if (sr == true) {
        last_seq = rcv_base; 
    }
    all_received[last_seq] = '\0';
    printf("Received data: %s\n", all_received);

    printf("Finished.\n");

    return 0;

} /* main() */

/**
 * @brief Configures and binds a socket to a local address.
 *
 * This function creates a socket using the provided address information and attempts
 * to bind it to a specified local address. If the socket creation or binding fails, 
 * an error message is printed, and the function returns a failure code.
 *
 * @param[in] bind_address A pointer to a struct addrinfo containing the address
 *                         information to which the socket should be bound.
 *                         This includes the family, type, protocol, and address.
 *
 * @return A valid SOCKET if the socket is successfully created and bound,
 *         or a failure code (1) if an error occurs during socket creation or binding.
 *
 */
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

    return socket_listen;


} /* configure_socket */
