#include "../../include/Utils.h"

// Get CPU time in seconds
double getCpuTime() {
    return static_cast<double>(clock()) / CLOCKS_PER_SEC;
}