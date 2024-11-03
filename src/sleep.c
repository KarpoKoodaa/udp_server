/******************************************
 * 
 * Filename:    sleep.c
 * 
 * Description: Pauses program execution for specified number of milliseconds
 * 
 * Notes:       Based on StackOverflow example: 
 *              https://stackoverflow.com/a/1157217
 * 
 * Copyright (c) 2024 Kariantti Laitala
 * Permission tba
 *******************************************/



#include <time.h>
#include <errno.h>

#include "../include/sleep.h"

/**
 * @brief Pauses the execution of a program for a specified number of milliseconds
 * 
 * @param msec number of milliseconds to sleep
 * @return int Return '0' if sleep completed successfully, or '-1' if there was an error
 */

int msleep(long msec) 
{
    struct timespec ts;
    int res;

    if (msec < 0) {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
} /* msleep() */
