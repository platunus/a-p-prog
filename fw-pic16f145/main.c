/*
 * a-p-prog FW for PIC 16F1454
 *
 * Copyright (c) 2024 @hanyazou
 *
 */
/*
 * USB CDC-ACM Demo
 *
 * This file may be used by anyone for any purpose and may be used as a
 * starting point making your own application using M-Stack.
 *
 * It is worth noting that M-Stack itself is not under the same license as
 * this file.
 *
 * M-Stack is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  For details, see sections 7, 8, and 9
 * of the Apache License, version 2.0 which apply to this file.  If you have
 * purchased a commercial license for this software from Signal 11 Software,
 * your commerical license superceeds the information in this header.
 *
 * Alan Ott
 * Signal 11 Software
 * 2014-05-12
 */

#include <common.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "usb.h"
#include "usb_config.h"
#include "usb_ch9.h"
#include "usb_cdc.h"
#include "hardware.h"

uint8_t uart_rx_buf[32];
uint8_t uart_rx_buf_len = 0;
uint8_t uart_rx_flush_timer = 0;
const uint8_t *uart_tx_buf;
uint8_t uart_tx_buf_len = 0;
uint8_t uart_tx_buf_head = 0;
uint8_t turn_on_led = 0;

uint8_t pp_res_buf[32];
uint8_t pp_res_buf_len = 0;

#define TICK_HZ 91
#define LED_TICKS 5
#define UART_FLUSH_TICKS 2

void pp_send_res_byte(uint8_t data);
void pp_flush_res_byte(void);

void uart_init(void)
{
    uint16_t brg_data;        // Baud Rate Generator setting data

    //brg_data = 0x0271;        //  19200 baud @ 48MHz
    brg_data = 0x0067;        // 115200 baud @ 48MHz

    TRIS(FW_PP_UART_RX) = 1;  // Input
    TRIS(FW_PP_UART_TX) = 0;  // Output
    TXSTA = 0x24;             // TX enable BRGH=1
    RCSTA = 0x90;             // Single Character RX
    SPBRG = (brg_data >> 0) & 0xff;
    SPBRGH = (brg_data >> 8) & 0xff;
    BAUDCON = 0x08;           // Use 16-bit baud rate generator (BRG16 = 1)
}

void timer_init(void)
{
    // Timer 1
    TMR1CS1 = 0;            // Clock source is Fosc/4
    TMR1CS0 = 0;
    T1CKPS1 = 0;            // Prescale 1:2
    T1CKPS0 = 1;
    TMR1ON = 1;             // Enable

    TMR1IF = 0;             // Clear interrupt flag

    TMR1IE = 1;             // Enable time 1 interrupt
    PEIE = 1;               // Enable peripheral interrupt
}

void led_init(void)
{
    // Initialize the LED (turn off)
    LAT(FW_PP_LED) = 1;
    TRIS(FW_PP_LED) = 0;
}

int main(void)
{
    /*
     * Initialize
     */
    ANSELA = 0;             // Disable all analog function
    ANSELC = 0;

    hardware_init();
    usb_init();
    uart_init();
    timer_init();
    led_init();

    pp_init(pp_send_res_byte);

    GIE = 1;                // Enable interrupt

    while (1) {
        /*
         * Blink LED if there are some activities
         */
        if (0 < turn_on_led) {
            LAT(FW_PP_LED) = 0;
        } else {
            LAT(FW_PP_LED) = 1;
        }

        if (!usb_is_configured())
            continue;

        /*
         * Handle PIC programmer packet
         */
        if (!usb_out_endpoint_halted(2) && usb_out_endpoint_has_data(2)) {
            const unsigned char *out_buf;
            size_t out_buf_len;

            out_buf_len = usb_get_out_buffer(2, &out_buf);
            pp_process(out_buf, out_buf_len);

            usb_arm_out_endpoint(2);
            turn_on_led = LED_TICKS;

            pp_flush_res_byte();
        }

        /*
         * Receive UART data and send to host via CDC(2)
         */
        if (PIR1bits.RCIF) {
            uint8_t c;
            if (RCSTAbits.OERR) {
                // reset the port in case of overrun error
                RCSTAbits.CREN = 0;
                c = RCREG;
                RCSTAbits.CREN = 1;
            } else {
                c = RCREG;
            }
            uart_rx_buf[uart_rx_buf_len++] = c;
            if (sizeof(uart_rx_buf) <= uart_rx_buf_len) {

                if (!usb_in_endpoint_halted(4)) {
                    usb_send_data(4, uart_rx_buf, uart_rx_buf_len);
                }
                uart_rx_buf_len = 0;
            } else {
                uart_rx_flush_timer = UART_FLUSH_TICKS;
            }
            turn_on_led = LED_TICKS;
        }
        if (uart_rx_flush_timer == 0 && 0 < uart_rx_buf_len) {
            usb_send_data(4, uart_rx_buf, uart_rx_buf_len);
            uart_rx_buf_len = 0;
        }

        /*
         * Receive CDC(2) data from host and send out via UART
         */
        if (uart_tx_buf_len == 0 && !usb_out_endpoint_halted(4) && usb_out_endpoint_has_data(4)) {
            uart_tx_buf_len = usb_get_out_buffer(4, &uart_tx_buf);
            uart_tx_buf_head = 0;
            turn_on_led = LED_TICKS;
        }
        if (PIR1bits.TXIF && 0 < uart_tx_buf_len) {
            TXREG = uart_tx_buf[uart_tx_buf_head++];
            if (uart_tx_buf_len <= uart_tx_buf_head) {
                uart_tx_buf_len = 0;
                usb_arm_out_endpoint(4);
            }
            turn_on_led = LED_TICKS;
        }
    }

    return 0;
}

void usb_send_data(uint8_t ep, uint8_t *data, size_t len)
{
    unsigned char *buf;

    while (usb_in_endpoint_busy(ep))
        ;
    buf = usb_get_in_buffer(ep);
    memcpy(buf, data, len);
    usb_send_in_buffer(ep, len);
}

void usb_send_byte(uint8_t data)
{
}

void pp_send_res_byte(uint8_t data)
{
    pp_res_buf[pp_res_buf_len++] = data;
    if (sizeof(pp_res_buf) <= pp_res_buf_len) {
        if (!usb_in_endpoint_halted(4)) {
            usb_send_data(2, pp_res_buf, pp_res_buf_len);
        }
        pp_res_buf_len = 0;
    }
}

void pp_flush_res_byte(void)
{
    usb_send_data(2, pp_res_buf, pp_res_buf_len);
    pp_res_buf_len = 0;
}

void usb_send_debug_string(char *str)
{
    usb_send_data(4, (uint8_t*)str, strlen(str));
}

void usb_send_debug_hex(uint32_t value)
{
    static const char *hex = "0123456789ABCDEF";
    uint8_t buf[8];
    unsigned int c = 0;
    do {
        buf[sizeof(buf) - ++c] = hex[value & 0xf];
        value >>= 4;
    } while (value);
    usb_send_data(4, &buf[sizeof(buf) - c], c);
}

/* Callbacks. These function names are set in usb_config.h. */
void app_set_configuration_callback(uint8_t configuration)
{

}

uint16_t app_get_device_status_callback()
{
    return 0x0000;
}

void app_endpoint_halt_callback(uint8_t endpoint, bool halted)
{

}

int8_t app_set_interface_callback(uint8_t interface, uint8_t alt_setting)
{
    return 0;
}

int8_t app_get_interface_callback(uint8_t interface)
{
    return 0;
}

void app_out_transaction_callback(uint8_t endpoint)
{

}

void app_in_transaction_complete_callback(uint8_t endpoint)
{

}

int8_t app_unknown_setup_request_callback(const struct setup_packet *setup)
{
    /* To use the CDC device class, have a handler for unknown setup
     * requests and call process_cdc_setup_request() (as shown here),
     * which will check if the setup request is CDC-related, and will
     * call the CDC application callbacks defined in usb_cdc.h. For
     * composite devices containing other device classes, make sure
     * MULTI_CLASS_DEVICE is defined in usb_config.h and call all
     * appropriate device class setup request functions here.
     */
    return (int8_t)process_cdc_setup_request(setup);
}

int16_t app_unknown_get_descriptor_callback(const struct setup_packet *pkt, const void **descriptor)
{
    return -1;
}

void app_start_of_frame_callback(void)
{

}

void app_usb_reset_callback(void)
{

}

/* CDC Callbacks. See usb_cdc.h for documentation. */

int8_t app_send_encapsulated_command(uint8_t interface, uint16_t length)
{
    return -1;
}

int16_t app_get_encapsulated_response(uint8_t interface,
                                      uint16_t length, const void **report,
                                      usb_ep0_data_stage_callback *callback,
                                      void **context)
{
    return -1;
}

int8_t app_set_comm_feature_callback(uint8_t interface,
                                     bool idle_setting,
                                     bool data_multiplexed_state)
{
    return -1;
}

int8_t app_clear_comm_feature_callback(uint8_t interface,
                                       bool idle_setting,
                                       bool data_multiplexed_state)
{
    return -1;
}

int8_t app_get_comm_feature_callback(uint8_t interface,
                                     bool *idle_setting,
                                     bool *data_multiplexed_state)
{
    return -1;
}

static struct cdc_line_coding line_coding =
{
    115200,
    CDC_CHAR_FORMAT_1_STOP_BIT,
    CDC_PARITY_NONE,
    8,
};

int8_t app_set_line_coding_callback(uint8_t interface,
                                    const struct cdc_line_coding *coding)
{
    line_coding = *coding;
    return 0;
}

int8_t app_get_line_coding_callback(uint8_t interface,
                                    struct cdc_line_coding *coding)
{
    /* This is where baud rate, data, stop, and parity bits are set. */
    *coding = line_coding;
    return 0;
}

int8_t app_set_control_line_state_callback(uint8_t interface,
                                           bool dtr, bool dts)
{
    return 0;
}

int8_t app_send_break_callback(uint8_t interface, uint16_t duration)
{
    /*
     * Universal Serial Bus Communications Class Subclass Specification for PSTN Devices Rev 1.2
     * 6.3.13 SendBreak
     * The wValue field contains the length of time, in milliseconds, of the break signal. If
     * wValue contains a value of FFFFh, then the device will send a break until another
     * SendBreak request is received with the wValue of 0000h.
     */
    if (interface == 2 && duration != 0) {
        /*
         * TXSTA: TRANSMIT STATUS AND CONTROL REGISTER
         * bit 3: SENDB: Send Break Character bit
         * Asynchronous mode:
         * 1 = Send Sync Break on next transmission (cleared by hardware upon completion)
         * 0 = Sync Break transmission completed
         */
        SENDB = 1;
        while (!PIR1bits.TXIF)
            ;
        TXREG = 0x00;
    }
    return 0;
}

void __interrupt() isr()
{
    // Periodic timer interrupt
    if (TMR1IF) {
        if (0 < turn_on_led)
            turn_on_led--;
        if (0 < uart_rx_flush_timer)
            uart_rx_flush_timer--;
        TMR1IF = 0;
    }

    usb_service();
}
