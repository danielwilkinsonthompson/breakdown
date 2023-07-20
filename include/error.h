/*=============================================================================
                                error.h
-------------------------------------------------------------------------------
standard error types for breakdown library

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#ifndef __error_h
#define __error_h

typedef enum error_t
{
    io_error = -6,
    index_error = -5,
    type_error = -4,
    value_error = -3,
    memory_error = -2,
    unspecified_error = -1,
    success = 0,
    warning = 1,
    buffer_underflow = 2,
} error;

/*-----------------------------------------------------------------------------
    error_message
-------------------------------------------------------------------------------
    human readable error messages for breakdown library
    e : the error to query
---
    returns : a string representation of the error
-----------------------------------------------------------------------------*/
const char *error_message(error e);

#endif