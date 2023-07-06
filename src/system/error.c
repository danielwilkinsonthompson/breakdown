/*=============================================================================
                                error.c
-------------------------------------------------------------------------------
standard error types for breakdown library

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/

#include "error.h"

char *error_message(error e)
{
    switch (e)
    {
    case success:
        return "No error";
    case unspecified_error:
        return "Unspecified error";
    case memory_error:
        return "Memory error";
    case value_error:
        return "Value error";
    case type_error:
        return "Type error";
    case index_error:
        return "Index error";
    case io_error:
        return "IO error";
    default:
        return "Unknown error";
    }
}
