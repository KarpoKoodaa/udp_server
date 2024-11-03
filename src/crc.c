#include <stdio.h>
#include <string.h>

#include "../include/crc.h"

/**
 * @brief precompute the CRC8 output remainders of each possible input
 * 
 */

void crcInit(void)
{
    crc remainder;

    for (int dividend = 0; dividend < 256; ++dividend) {
        remainder = dividend << (WIDTH - 8);

        for (uint8_t bit = 8; bit > 0; --bit) {
            if (remainder & TOPBIT) {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            }
            else {
                remainder = (remainder << 1);
            }
        }

        crcTable[dividend] = remainder;

    }
}   /* crcInit */

/**
 * @brief Compute the CRC of given message
 * @note  crcInit() must be called first
 * @param message '8'-bit message data for which the CRC is calculated
 * @param nBytes The number of bytes in the message
 * @return crc The computed CRC value (uint8_t)
 */

crc crcFast (uint8_t const message[], int nBytes) 
{
    uint8_t data;
    crc remainder = 0;

    for (int byte = 0; byte < nBytes; ++byte) {
        data = message[byte] ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }

    return (remainder);
}   /* crcFast() */
