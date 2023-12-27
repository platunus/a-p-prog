#include "pp3.h"

enum {
    OP_IO_MCLR = 99,
    OP_IO_DAT,
    OP_IO_CLK,
    OP_READ_ISP,
    OP_WRITE_ISP,
    OP_READ_ISP_BITS,
    OP_WRITE_ISP_BITS,
    OP_DELAY_US,
    OP_DELAY_10US,
    OP_DELAY_MS,
    OP_REPLY,
    OP_NONE = 0xff,
};

static uint8_t buf[278];
static int buf_len = 0;
static int res_len = 0;

void putByte(int byte);
int getByte(void);

void pp_ops_init(void)
{
    buf_len = 0;
    res_len = 0;
}

int pp_ops_io_mclr(int v)
{
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    buf[buf_len++] = OP_IO_MCLR;
    buf[buf_len++] = (v) ? 0x01 : 0x00;
    return 0;
}

int pp_ops_io_dat_in(void)
{
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    buf[buf_len++] = OP_IO_DAT;
    buf[buf_len++] = 0;
    return 0;
}

int pp_ops_io_dat_out(int v)
{
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    buf[buf_len++] = OP_IO_DAT;
    buf[buf_len++] = (0x02 | ((v) ? 0x01 : 0x00));
    return 0;
}

int pp_ops_io_clk_in(void)
{
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    buf[buf_len++] = OP_IO_CLK;
    buf[buf_len++] = 0;
    return 0;
}

int pp_ops_io_clk_out(int v)
{
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    buf[buf_len++] = OP_IO_CLK;
    buf[buf_len++] = (0x02 | ((v) ? 0x01 : 0x00));
    return 0;
}

int pp_ops_read_isp(int n)
{
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    buf[buf_len++] = OP_READ_ISP;
    buf[buf_len++] = n;
    res_len += n;
    return 0;
}

int pp_ops_write_isp(uint8_t *v, int n)
{
    if (sizeof(buf) < buf_len + 2 + n) {
        return -1;
    }
    buf[buf_len++] = OP_WRITE_ISP;
    buf[buf_len++] = n;
    while (0 < n--) {
        buf[buf_len++] = *v++;
    }

    return 0;
}

int pp_ops_delay_us(int n)
{
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    if (n < 256) {
        buf[buf_len++] = OP_DELAY_US;
    } else {
        buf[buf_len++] = OP_DELAY_10US;
        n /= 10;
    }
    buf[buf_len++] = n;
    return 0;
}

int pp_ops_delay_ms(int n)
{
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    buf[buf_len++] = OP_DELAY_MS;
    buf[buf_len++] = n;
    return 0;
}

int pp_ops_reply(uint8_t v)
{
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    buf[buf_len++] = OP_REPLY;
    buf[buf_len++] = v;
    res_len++;
    return 0;
}

int pp_ops_exec(uint8_t *v, int *n)
{
    verbose_print("%s: buf_len=%d, res_len=%d, *n=%d\n", __func__, buf_len, res_len, *n);
    if (*n < res_len) {
        return -1;
    }
    putByte(0x80);
    putByte(buf_len & 0xff);
    for (int i = 0; i < buf_len; i++) {
        // Without this delay Arduino Leonardo's USB serial might be stalled
        sleep_us(5);
        putByte(buf[i]);
    }
    for (int i = 0; i < buf_len && i < 2; i++) {
        verbose_print("%3d: -> %02x\n", i, buf[i]);
    }
    *n = res_len;
    for (int i = 0; i < res_len; i++) {
        v[i] = getByte();
    }
    for (int i = 0; i < *n && i < 4; i++) {
        verbose_print("%3d: <- %02x\n", i, v[i]);
    }
    return 0;
}

int pp_ops_write_isp_8(uint8_t v)
{
    uint8_t buf[1];
    buf[0] = v;
    return pp_ops_write_isp(buf, 1);
}

int pp_ops_write_isp_24(uint32_t v)
{
    int res;
    uint8_t buf[3];

    v <<= 1;
    buf[0] = (v >> 16) & 0xff;
    buf[1] = (v >>  8) & 0xff;
    buf[2] = (v >>  0) & 0xff;
    return pp_ops_write_isp(buf, 3);
}
