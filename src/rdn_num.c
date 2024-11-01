#include <stdio.h>
#include <stdlib.h>


double rand_number(void)
{
    int max = 10;
    int min = 1;

    int rd_num = rand() % (max - min + 1) + min;
    
    return (float)rd_num / 10;
}
