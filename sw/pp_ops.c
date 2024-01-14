#include <string.h>
#include "pp3.h"

static uint8_t buf[278];
static int buf_len = 0;
static int res_len = 0;
static uint8_t pp_params[PP_PARAM_NUM_PARAMS] = { 0 };

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

int pp_ops_isp_read_8_msb(void)
{
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    buf[buf_len++] = OP_READ_ISP;
    buf[buf_len++] = 1;
    res_len += 1;
    return 0;
}

int pp_ops_isp_send_8_msb(uint8_t v)
{
    if (sizeof(buf) < buf_len + 2 + 1) {
        return -1;
    }
    buf[buf_len++] = OP_WRITE_ISP;
    buf[buf_len++] = 1;
    buf[buf_len++] = v;

    return 0;
}

int pp_ops_isp_send_24_msb(uint32_t v)
{
    return pp_ops_write_isp_24(v);
}

int pp_ops_read_isp_bits(int n)
{
    int bytes = (pp_params[PP_PARAM_DATA_LEN] + 7) / 8;
    n /= bytes;
    if (sizeof(buf) < buf_len + 2) {
        return -1;
    }
    buf[buf_len++] = OP_READ_ISP_BITS;
    buf[buf_len++] = n;
    res_len += (n * bytes);
    return 0;
}

int pp_ops_write_isp_bits(uint8_t *v, int n)
{
    int bytes = (pp_params[PP_PARAM_DATA_LEN] + 7) / 8;
    n /= bytes;
    if (sizeof(buf) < buf_len + 2 + n * bytes) {
        return -1;
    }
    buf[buf_len++] = OP_WRITE_ISP_BITS;
    buf[buf_len++] = n;
    while (0 < n--) {
        for (int i = 0; i < bytes; i++) {
            buf[buf_len++] = *v++;
        }
    }

    return 0;
}

int pp_ops_isp_send_msb_multi(uint32_t v, int len, int n)
{
    pp_ops(param_reset());
    pp_ops(param_set(PP_PARAM_CLK_DELAY, 1));
    pp_ops(param_set(PP_PARAM_DATA_LEN, len));

    int bytes = (pp_params[PP_PARAM_DATA_LEN] + 7) / 8;
    if (sizeof(buf) < buf_len + 2 + len * bytes) {
        return -1;
    }
    buf[buf_len++] = OP_WRITE_ISP_BITS;
    buf[buf_len++] = n;
    v <<= (32 - len);
    while (0 < n--) {
        uint32_t t = v;
        for (int i = 0; i < bytes; i++) {
            buf[buf_len++] = (t >> 24) & 0xff;
            t <<= 8;
        }
    }

    return 0;
}

int pp_ops_isp_send_multi(uint32_t v, int len, int n)
{
    return pp_ops_isp_send_msb_multi(pp_util_revert_bit_order(v, len), len, n);
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

int pp_ops_param_set(int param, int value)
{
    if (sizeof(buf) < buf_len + 3) {
        return -1;
    }
    buf[buf_len++] = OP_PARAM_SET;
    buf[buf_len++] = param;
    buf[buf_len++] = value;
    pp_params[param] = value;
    return 0;
}

int pp_ops_param_reset()
{
    if (sizeof(buf) < buf_len + 1) {
        return -1;
    }
    buf[buf_len++] = OP_PARAM_RESET;
    memset(pp_params, 0, sizeof(pp_params));
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
        if (!(pp_fw_caps & PP_CAP_ASYNC_WRITE)) {
            // Without this delay Arduino Leonardo's USB serial might be stalled
            sleep_us(5);
        }
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
