/******************************************************************************
  * @file           : rdt.h
  * @brief          : TODO 
******************************************************************************/

#ifndef __RDT_H__
#define __RDT_H__
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#include "../include/sleep.h"
#include "../include/rdn_num.h"

typedef struct Rdt_variables
{
    /* data */
    float drop_probability;
    float delay_probability;
    float error_probability;
    uint16_t delay_ms;
    uint16_t rdt;     // Reliable data transfer version (1.0, 2.0, 2.1, 2.2 or 3.0)
} Rdt_variables;

// typedef struct Rdt_packet
// {
//     // TODO: Packet
// } Rdt_packet;

bool process_packet (char *read, size_t bytes_received, struct Rdt_variables vars);




#endif /* __RDT_H__ */
