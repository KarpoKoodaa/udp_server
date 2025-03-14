/******************************************
 * 
 * Filename:    rdn_num.c
 * 
 * Description: Generates random numbers between 0.1 - 1.0
 * 
 * Copyright (c) 2024 Kariantti Laitala
 * Permission tba
 *******************************************/


#include <stdio.h>
#include <stdlib.h>

#include "../include/rdn_num.h"

double rand_number(void)
{
    int max = 10;
    int min = 1;

    int rd_num = rand() % (max - min + 1) + min;
    
    return (double)rd_num / 10;
}   /* rand_number() */
