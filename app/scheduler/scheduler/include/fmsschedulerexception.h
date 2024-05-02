/*
 * fmsschedulerexception.h
 *
 *  Created on: 21 May 2014
 *      Author: uwaqas
 * Modified by
 */

#ifndef FMSSCHEDULEREXCEPTION_H_
#define FMSSCHEDULEREXCEPTION_H_

#include "pch/containers.hpp"

#include <stdexcept>

class FmsSchedulerException : public std::runtime_error {

public:
    explicit FmsSchedulerException(const std::string &what) : std::runtime_error(what) {}
};

struct ParseException : FmsSchedulerException
{
    explicit ParseException(const std::string &what) : FmsSchedulerException(what) {}
};

#endif /* FMSSCHEDULEREXCEPTION_H_ */
