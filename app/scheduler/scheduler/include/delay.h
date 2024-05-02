/*@file
 * @author  Umar Waqas <u.waqas@tue.nl>
 * @version 0.1
 *
 * Modified by Joost van Pinxten (joost.vanpinxten@cpp.canon)
 */

#ifndef DELAY_H_
#define DELAY_H_

#include <cstdint>

/**
 * The constraints are defined in terms of 'delay', which are fixed-precision
 * numbers represented by (64-bit) integers.
 */
using delay = std::int64_t;

#endif /* DELAY_H_ */
