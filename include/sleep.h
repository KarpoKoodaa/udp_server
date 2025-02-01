/******************************************************************************
  * @file           : sleep.h
  * @brief          : Sleep or delay execution for milliseconds
*/

#ifndef __SLEEP_H__
#define __SLEEP_H__

/**
 * @brief Pauses the execution of a program for a specified number of milliseconds
 * 
 * @param msec number of milliseconds to sleep
 * @return int Return '0' if sleep completed successfully, or '-1' if there was an error
 */
int msleep(long msec);

#endif /* __SLEEP_H__ */
