#ifndef THREAD_MUSIC_UTILS_H
#define THREAD_MUSIC_UTILS_H

#include <ctime>

/**
 * Returns the CPU time used by the current process
 * 
 * Used to detect when a thread is being scheduled by comparing
 * with wall clock time - essential for thread scheduling sonification
 * 
 * @return CPU time in seconds as a double
 */
double getCpuTime();

#endif // THREAD_MUSIC_UTILS_H