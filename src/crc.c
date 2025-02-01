/******************************************
 * 
 * Filename:    crc.c
 * 
 * Description: Fast implementation of CRC8
 * 
 * Notes:       Based on Michael Barr's CRC8 examples: 
 *              https://barrgroup.com/blog/crc-series-part-3-crc-implementation-code-cc
 * 
 * Copyright (c) 2024 Kariantti Laitala
 * Permission tba
 *******************************************/

#include <stdio.h>
#include <string.h>

#include "../include/crc.h"

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
