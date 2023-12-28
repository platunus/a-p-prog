#ifndef __FW_PP_16F145_COMMON_H__
#define __FW_PP_16F145_COMMON_H__

#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 48000000UL

#define FW_PP_ISP_MCLR     C3
#define FW_PP_LED          C2
#define FW_PP_ISP_CLK      A5
#define FW_PP_ISP_DAT      A4

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
void usb_send_byte(uint8_t data);
void usb_send_debug_string(char *str);
void usb_send_debug_hex(uint32_t value);
void pp_init(void (*sendbackp)(uint8_t data));
void pp_process(const uint8_t *data, size_t len);

#endif  // __FW_PP_16F145_COMMON_H__
