/*=============================================================================
                                vector_test.c
-------------------------------------------------------------------------------
simple test of analysis/vector

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

TODO:
    - Comment code
-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "vector.h"
#include "csv.h"

int main(int argc, char *argv[])
{
    vect **data;
    uint32_t cols;
    vect *voltage;
    vdata av;

    if (argc <= 1)
    {
        fprintf(stderr, "No file specified\r\n");
        return -1;
    }

    data = vect_read_csv(argv[1], &cols);
    if (data == NULL)
    {
        fprintf(stderr, "Could not read csv file %s\r\n", argv[1]);
    }

    if (cols >= 2)
        voltage = data[1];

    if (vect_invalid(voltage))
    {
        fprintf(stderr, "CSV data appears to be invalid");
        return -2;
    }

    vect_debug_value(voltage);
    vect_debug_command(av = vect_mean(voltage));
    fprintf(stdout, "%0.5e\r\n\n\n", av);
}