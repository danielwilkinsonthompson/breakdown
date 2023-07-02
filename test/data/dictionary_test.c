#include <stdio.h>
#include <string.h>
#include "dictionary.h"

int main(int argc, char *argv[])
{
    dictionary *d = dictionary_new(10);
    dictionary_insert(d, "key1", "value1");
    dictionary_insert(d, "key2", "value2");
    dictionary_insert(d, "pineapple", "value3");
    dictionary_print(d);
}