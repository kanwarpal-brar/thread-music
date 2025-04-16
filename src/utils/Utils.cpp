#include "../../include/Utils.h"

/**
 * Returns the CPU time used by the current process
 * 
 * This function is key to the thread scheduling detection mechanism.
 * By comparing CPU time (which only advances when a thread is actively running)
 * with wall clock time (which always advances), threads can detect when 
 * they're being scheduled by the operating system.
 * 
 * @return CPU time in seconds as a double
 */
double getCpuTime() {
    return static_cast<double>(clock()) / CLOCKS_PER_SEC;  // thank you xcode
}