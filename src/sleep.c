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
