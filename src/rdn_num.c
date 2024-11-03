#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Random number generator
 * @note  Random numbers are between 0.1 - 1.0
 * @return random number divided by '10'
 */

double rand_number(void)
{
    int max = 10;
    int min = 1;

    int rd_num = rand() % (max - min + 1) + min;
    
    return (double)rd_num / 10;
}   /* rand_number() */
