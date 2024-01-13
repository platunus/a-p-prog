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

#ifndef __FW_PP_OPS_H__
#define __FW_PP_OPS_H__

#define PP_PROTO_TYPE_PPROG 0x88
#define PP_PROTO_MAJOR_VERSION 1
#define PP_PROTO_MINOR_VERSION 0

#define PP_CAP_LEGACY   (1 << 0)
#define PP_CAP_PP_OPS   (1 << 1)
#define PP_CAP_ASYNC_WRITE  (1 << 2)

enum {
    OP_NONE,
    OP_IO_MCLR,
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
    OP_PARAM_SET,
    OP_PARAM_RESET,
};

enum {
    PP_PARAM_CLK_DELAY,
    PP_PARAM_CMD1,
    PP_PARAM_CMD1_LEN,
    PP_PARAM_DELAY1,
    PP_PARAM_CMD2,
    PP_PARAM_CMD2_LEN,
    PP_PARAM_DELAY2,
    PP_PARAM_PREFIX_LEN,
    PP_PARAM_DATA_LEN,
    PP_PARAM_POSTFIX_LEN,
    PP_PARAM_DELAY3,
    PP_PARAM_NUM_PARAMS  // number of parameters
};

#endif  // __FW_PP_OPS_H__
