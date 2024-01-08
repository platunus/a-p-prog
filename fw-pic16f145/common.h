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

#ifndef __FW_PP_16F145_COMMON_H__
#define __FW_PP_16F145_COMMON_H__

#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 48000000UL

#define FW_PP_ISP_MCLR     C3
#define FW_PP_LED          C2
#define FW_PP_ISP_CLK      A5
#define FW_PP_ISP_DAT      A4
#define FW_PP_UART_TX      C4
#define FW_PP_UART_RX      C5

#define PORT_CAT(x, y) PORT_CAT_(x, y)
#define PORT_CAT_(x, y) x ## y
#define ANSEL(port) PORT_CAT(ANSEL, port)
#define TRIS(port) PORT_CAT(TRIS, port)
#define LAT(port) PORT_CAT(LAT, port)
#define PORT(port) PORT_CAT(PORT, port)

#ifndef PORTA4
#define PORTA4 (PORTAbits.RA4)
#endif

void usb_send_data(uint8_t ep, uint8_t *data, size_t len);
void usb_send_debug_string(char *str);
void usb_send_debug_hex(uint32_t value);
void pp_init(void (*sendbackp)(uint8_t data));
void pp_process(const uint8_t *data, size_t len);

#endif  // __FW_PP_16F145_COMMON_H__
