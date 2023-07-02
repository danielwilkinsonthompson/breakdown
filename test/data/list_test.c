/*=============================================================================
                                list_test.c
-------------------------------------------------------------------------------
simple test of linked list

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "list.h"

int main(int argc, char *argv[])
{
    char a = 'A';
    char b = 'B';
    char c = 'C';
    char *ret_ptr;
    char ret_val;
    list *test = list_init();

    list_push(test, &a);
    list_push(test, &b);
    list_push(test, &c);

    list_print(test);
    fprintf(stderr, "list_item(list, %d) return %C\r\n", 2, *((char *)list_item(test, 2)));

    fprintf(stderr, "list_remove(list, 2)\r\n");
    list_remove(test, 2);

    fprintf(stderr, "Pop returns %C\r\n", *((char *)list_pop(test)));
    list_print(test);

    return EXIT_SUCCESS;
}
