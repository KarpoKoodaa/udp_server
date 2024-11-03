/******************************************************************************
  * @file           : crc.h
  * @brief          : return crc value.
*/

#ifndef __CRC_H__
#define __CRC_H__

#include <stdint.h>

typedef uint8_t crc;
extern crc crcTable[256]; 

#define WIDTH   (8 * sizeof(crc))
#define TOPBIT  (1 << (WIDTH -1))

#define POLYNOMIAL 0x07  /* 11011 followed by 0's */

void crcInit(void);
crc crcFast (uint8_t const message[], int nBytes);

#endif /* __CRC_H__ */
