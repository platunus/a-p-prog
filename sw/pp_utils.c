#include "pp3.h"
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

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
        if (i < size && 0x20 <= buf[i] && buf[i] <= 0x7e) {
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

void pp_util_flush_printf(FILE* f, char *fmt, ...)
{
    char buf[500];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    fprintf(f,"%s", buf);
    fflush(f);
    va_end(va);
}

//  get line replacement
size_t pp_util_getline(char **lineptr, size_t *n, FILE *stream)
{
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL)
        return -1;
    if (stream == NULL)
        return -1;
    if (n == NULL)
        return -1;

    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF)
        return -1;
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while (c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }
    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;
    return p - bufptr - 1;
}

void sleep_ms(int num)
{
    struct timespec tspec;
    tspec.tv_sec = num / 1000;
    tspec.tv_nsec = (num % 1000) * 1000000;
    nanosleep(&tspec, 0);
}

void sleep_us(int num)
{
    struct timespec tspec;
    tspec.tv_sec = num / 1000000;
    tspec.tv_nsec = (num % 1000000) * 1000;
    nanosleep(&tspec, 0);
}
