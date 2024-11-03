#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef uint8_t crc;
crc crcTable[256]; // Move to header

#define WIDTH   (8 * sizeof(crc))
#define TOPBIT  (1 << (WIDTH -1))

#define POLYNOMIAL 0x07  /* 11011 followed by 0's */

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
}

crc crcFast (uint8_t const message[], int nBytes) 
{
    uint8_t data;
    crc remainder = 0;

    for (int byte = 0; byte < nBytes; ++byte) {
        data = message[byte] ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }

    return (remainder);
}


