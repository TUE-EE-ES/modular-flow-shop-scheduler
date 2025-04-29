/*
* fmsschedulerexception.h
*
*  Created on: 21 May 2014
*      Author: uwaqas
* Modified by
*/

#ifndef FMS_SCHEDULER_EXCEPTION_HPP
#define FMS_SCHEDULER_EXCEPTION_HPP

#include <stdexcept>
#include <string>

/**
* @class FmsSchedulerException
* @brief Custom exception class for FMS Scheduler.
*
* This class is a custom exception class that inherits from std::runtime_error.
* It is used to throw exceptions specific to the FMS Scheduler.
*/
class FmsSchedulerException : public std::runtime_error {

public:
   /**
    * @brief Constructor for the FmsSchedulerException class.
    *
    * @param what The error message for the exception.
    */
   explicit FmsSchedulerException(const std::string &what) : std::runtime_error(what) {}
};

/**
* @struct ParseException
* @brief Custom exception struct for parsing errors.
*
* This struct is a custom exception that inherits from FmsSchedulerException.
* It is used to throw exceptions specific to parsing errors in the FMS Scheduler.
*/
struct ParseException : FmsSchedulerException
{
   /**
    * @brief Constructor for the ParseException struct.
    *
    * @param what The error message for the exception.
    */
   explicit ParseException(const std::string &what) : FmsSchedulerException(what) {}
};

#endif /* FMS_SCHEDULER_EXCEPTION_HPP */