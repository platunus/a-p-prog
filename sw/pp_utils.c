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

void pp_util_hexdump(const char *header, uint32_t addr_offs, const void *data, int size)
{
    char chars[17];
    const uint8_t *buf = data;
    for (unsigned int i = 0; i < ((size + 15) & ~0xfU); i++) {
        if ((i % 16) == 0)
            printf("%s%06lx:", header, (unsigned long)addr_offs + i);
        if (i < size) {
            printf(" %02x", buf[i]);
        } else {
            printf("   ");
        }
        if (0x20 <= buf[i] && buf[i] <= 0x7e) {
            chars[i % 16] = buf[i];
        } else
        if (i < size) {
            chars[i % 16] = '.';
        } else {
            chars[i % 16] = ' ';
        }
        if ((i % 16) == 15) {
            chars[16] = '\0';
            printf(" %s\n\r", chars);
        }
    }
}
