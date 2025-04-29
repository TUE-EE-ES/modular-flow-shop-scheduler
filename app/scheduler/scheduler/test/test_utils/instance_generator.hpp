#ifndef TEST_UTILS_INSTANCE_GENERATOR_HPP
#define TEST_UTILS_INSTANCE_GENERATOR_HPP

#include <fms/problem/flow_shop.hpp>

#include <map>

fms::problem::Instance createHomogeneousCase(fms::delay load_time,
                                             fms::delay print_1_time,
                                             fms::delay print_2_time,
                                             fms::delay unload_time,
                                             fms::delay buffer_min,
                                             fms::delay buffer_max,
                                             unsigned int n_pages);

#endif // TEST_UTILS_INSTANCE_GENERATOR_HPP
