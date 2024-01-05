#include "pp3.h"

uint32_t pp_util_revert_bit_order(uint32_t v, int n)
{
    uint32_t t = 0;
    for (int i = 0; i < n; i++) {
        t = (t << 1) | (v & 1);
        v >>= 1;
    }
    return t;
}
