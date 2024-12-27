
#include "../include/rdt.h"

bool process_packet (char *read, size_t bytes_received, struct Rdt_variables vars)
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
