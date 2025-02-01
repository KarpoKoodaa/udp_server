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

/**
 * @brief precompute the CRC8 output remainders of each possible input
 * 
 */
void crcInit(void);

/**
 * @brief Compute the CRC of given message
 * @note  crcInit() must be called first
 * @param message '8'-bit message data for which the CRC is calculated
 * @param nBytes The number of bytes in the message
 * @return crc The computed CRC value (uint8_t)
 */
crc crcFast (uint8_t const message[], int nBytes);

#endif /* __CRC_H__ */
