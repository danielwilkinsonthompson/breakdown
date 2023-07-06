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
} error;

// struct error_description_t
// {
//     error code;
//     char *message;
// } error_description[] = {
//     {success, "No error"},
//     {e_failure, "Unspecified error"},
//     {e_memory, "Memory error"}
//     // {E_FILE_NOT_FOUND, "File not found"},
// };

char *error_message(error e);

#endif