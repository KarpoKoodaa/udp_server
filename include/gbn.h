/******************************************************************************
  * @file           : gbn.h
  * @brief          : TODO 
******************************************************************************/

#ifndef __GBN_H__
#define __GBN_H__
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#include "../include/sleep.h"
#include "../include/rdn_num.h"
#include "../include/crc.h"


bool gbn_process_packet (char *read, long bytes_received, int expectedseqnum);
int gbn_make_packet(char *packet, uint8_t expectedseqnum);

#endif /* __GBN_H__ */
