/*
 * a-p-prog FW for PIC 16F1454
 *
 * Copyright (c) 2024 @hanyazou
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

uint8_t pp_params[PP_PARAM_NUM_PARAMS] = { 0 };

void send_bits(uint8_t d, uint8_t len)
{
    if (len)
        verbose_print("%s: %02x (%d bits)", __func__, d, len);
    ISP_DAT_OUT;
    for (uint8_t i = 0; i < len; i++) {
        ISP_DAT(d & 0x80);
        fw_pp_ops_clk_delay();
        ISP_CLK(1);
        // fw_pp_ops_clk_delay();
        d <<= 1;
        ISP_CLK(0);
        ISP_DAT(0);
    }
}

void dummy_locks(uint8_t n)
{
    for (uint8_t i = 0; i < n; i++) {
        fw_pp_ops_clk_delay();
        ISP_CLK(1);
        fw_pp_ops_clk_delay();
        ISP_CLK(0);
    }
}

void exec_ops(uint8_t *ops, int len)
{
    uint8_t i, n, t, d;
    uint8_t *endp = &ops[len];
    uint8_t txbuf[4];
    uint8_t txbuf_len;
    uint16_t n10;

    while (ops < endp) {
        n = ops[1];
        switch (ops[0]) {
        case OP_IO_MCLR:
            verbose_print("0x80: exec_ops:        IO_MCLR %02x", n);
            ISP_MCLR(n & 0x01);
            break;
        case OP_IO_DAT:
            verbose_print("0x80: exec_ops:         IO_DAT %02x", n);
            if (n & 0x02) {
                ISP_DAT(n & 0x01);
                ISP_DAT_OUT;
            } else {
                ISP_DAT_IN;
            }
            break;
        case OP_IO_CLK:
            verbose_print("0x80: exec_ops:         IO_CLK %02x", n);
            if (n & 0x02) {
                ISP_CLK(n & 0x01);
                ISP_CLK_OUT;
            } else {
                ISP_CLK_IN;
            }
            break;
        case OP_READ_ISP:
            verbose_print("0x80: exec_ops:       READ_ISP %02x %02x %02x %02x", n,
                          ops[2], ops[3], ops[4]);
            d = 0;
            ISP_DAT_IN;
            while (0 < n--) {
                for (i = 0; i < 8; i++) {
                    ISP_CLK(1);
                    fw_pp_ops_clk_delay();
                    ISP_CLK(0);
                    fw_pp_ops_clk_delay();
                    d = (uint8_t)(d << 1) | ((ISP_DAT_V) ? 1 : 0);
                }
                usart_tx_b(d);
            }
            break;
        case OP_WRITE_ISP:
            verbose_print("0x80: exec_ops:      WRITE_ISP %02x %02x %02x %02x", n,
                          ops[2], ops[3], ops[4]);
            ISP_DAT_OUT;
            while (0 < n--) {
                d = ops[2];
                ops++;
                for (i = 0; i < 8; i++) {
                    ISP_DAT(d & 0x80);
                    fw_pp_ops_clk_delay();
                    ISP_CLK(1);
                    fw_pp_ops_clk_delay();
                    d <<= 1;
                    ISP_CLK(0);
                    ISP_DAT(0);
                }
            }
            fw_pp_ops_delay(3);  // XXX
            break;
        case OP_READ_ISP_BITS:
            verbose_print("0x80: exec_ops:  READ_ISP_BITS %02x %02x", n, ops[2]);
            while (0 < n--) {

                send_bits(pp_params[PP_PARAM_CMD1], pp_params[PP_PARAM_CMD1_LEN]);
                fw_pp_ops_delay(pp_params[PP_PARAM_DELAY1]);

                ISP_DAT_IN;
                dummy_locks(pp_params[PP_PARAM_PREFIX_LEN]);
                d = 0;
                txbuf_len = 0;
                for (i = 0; i < pp_params[PP_PARAM_DATA_LEN]; i++) {
                    // fw_pp_ops_clk_delay();
                    ISP_CLK(1);
                    fw_pp_ops_clk_delay();
                    ISP_CLK(0);
                    d = (uint8_t)(d << 1) | ((ISP_DAT_V) ? 1 : 0);
                    if ((i % 8) == 7) {
                        txbuf[txbuf_len++] = d;
                        d = 0;
                    }
                }
                if ((i % 8) != 0) {
                    d <<= (8 - (i % 8));  // Shift to MSB side if less than 8 bits
                    txbuf[txbuf_len++] = d;
                }
                dummy_locks(pp_params[PP_PARAM_POSTFIX_LEN]);
                fw_pp_ops_delay(pp_params[PP_PARAM_DELAY2]);

                send_bits(pp_params[PP_PARAM_CMD2], pp_params[PP_PARAM_CMD2_LEN]);
                fw_pp_ops_delay(pp_params[PP_PARAM_DELAY3]);

                for (i = 0; i < txbuf_len; i++) {
                    verbose_print("0x80: exec_ops:  READ_ISP_BITS tx: %02x %02x", n, txbuf[i]);
                    usart_tx_b(txbuf[i]);
                }
            }
            break;
        case OP_WRITE_ISP_BITS:
            verbose_print("0x80: exec_ops: WRITE_ISP_BITS %02x %02x", n, ops[2]);
            while (0 < n--) {

                send_bits(pp_params[PP_PARAM_CMD1], pp_params[PP_PARAM_CMD1_LEN]);
                fw_pp_ops_delay(pp_params[PP_PARAM_DELAY1]);

                ISP_DAT(0);
                dummy_locks(pp_params[PP_PARAM_PREFIX_LEN]);
                d = ops[2];
                ops++;
                for (i = 0; i < pp_params[PP_PARAM_DATA_LEN]; i++) {
                    ISP_DAT(d & 0x80);
                    d <<= 1;
                    // fw_pp_ops_clk_delay();
                    ISP_CLK(1);
                    if ((i % 8) == 7) {
                        d = ops[2];
                        verbose_print("0x80: exec_ops: WRITE_ISP_BITS rx: %02x %02x", n, d);
                        ops++;
                    }
                    // fw_pp_ops_clk_delay();
                    ISP_CLK(0);
                }
                if ((i % 8) == 0) {
                    ops--;
                }
                ISP_DAT(0);
                dummy_locks(pp_params[PP_PARAM_POSTFIX_LEN]);
                fw_pp_ops_delay(pp_params[PP_PARAM_DELAY2]);

                send_bits(pp_params[PP_PARAM_CMD2], pp_params[PP_PARAM_CMD2_LEN]);
                fw_pp_ops_delay(pp_params[PP_PARAM_DELAY3]);
            }
            break;
        case OP_DELAY_US:
            verbose_print("0x80: exec_ops:       DELAY_US %d", n);
            fw_pp_ops_delay(n);
            break;
        case OP_DELAY_10US:
            verbose_print("0x80: exec_ops:     DELAY_10US %d", n);
            while (0 < n--)
                fw_pp_ops_delay(10);
            break;
        case OP_DELAY_MS:
            verbose_print("0x80: exec_ops:       DELAY_MS %d", n);
            n10 = n * 10;
            while (0 < n10--)
                fw_pp_ops_delay(100);
            break;
        case OP_REPLY:
            verbose_print("0x80: exec_ops:         REPLAY %02x", n);
            usart_tx_b(n);
            break;
        case OP_PARAM_SET:
            verbose_print("0x80: exec_ops:      PARAM_SET pp_params[%02x] = %02x", n, ops[2]);
            pp_params[n] = ops[2];
            ops++;
            break;
        case OP_PARAM_RESET:
            verbose_print("0x80: exec_ops:    PARAM_RESET", n);
            memset(pp_params, 0, sizeof(pp_params));
            ops -= 1;  // no operands
            break;
        default:
            verbose_print("0x80: exec_ops:            ??? %02x", n);
            break;
        }
        ops += 2;
    }
}
