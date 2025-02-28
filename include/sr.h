/******************************************************************************
  * @file           : sr.h
  * @brief          : TODO 
******************************************************************************/

#ifndef __SR_H__
#define __SR_H__
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#include "../include/sleep.h"
#include "../include/rdn_num.h"
#include "../include/crc.h"

#define MAX_BUFFER_SIZE 50

typedef struct {
    bool received[MAX_BUFFER_SIZE]; 
    char data[MAX_BUFFER_SIZE];
} sr_receive_buffer_t;

int sr_process_packet (char *read, long bytes_received);
int sr_make_packet(char *packet, uint8_t seqnum);
int deliver_data(sr_receive_buffer_t buffer, char *data, int recv_base);

#endif
