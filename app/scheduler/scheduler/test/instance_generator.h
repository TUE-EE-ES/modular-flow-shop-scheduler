#ifndef COMMON_UTILS
#define COMMON_UTILS

#include <FORPFSSPSD/FORPFSSPSD.h>
#include <map>

FORPFSSPSD::Instance createHomogeneousCase(delay load_time, delay print_1_time, delay print_2_time, delay unload_time, delay buffer_min, delay buffer_max, unsigned int n_pages);

#endif // COMMON_UTILS
