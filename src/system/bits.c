#include <stdint.h>
#include "bits.h"

uint8_t flip_uint8_t(uint8_t x)
{
    uint8_t out = 0;
    for (uint8_t bit_no = 0; bit_no < 8; bit_no++)
    {
        out |= ((x >> bit_no) & 1) << (7 - bit_no);
    }
    return out;
}

uint16_t flip_uint16_t(uint16_t x)
{
    uint16_t out = 0;
    for (uint8_t bit_no = 0; bit_no < 16; bit_no++)
    {
        out |= ((x >> bit_no) & 1) << (15 - bit_no);
    }
    return out;
}

uint32_t flip_uint32_t(uint32_t x)
{
    uint32_t out = 0;
    for (uint8_t bit_no = 0; bit_no < 32; bit_no++)
    {
        out |= ((x >> bit_no) & 1) << (31 - bit_no);
    }
    return out;
}

uint64_t flip_uint64_t(uint64_t x)
{
    uint64_t out = 0;
    for (uint8_t bit_no = 0; bit_no < 64; bit_no++)
    {
        out |= ((x >> bit_no) & 1) << (63 - bit_no);
    }
    return out;
}