/*
 * pp programmer, for SW 0.99 and higher
 */

#include <avr/io.h>
#include <util/delay.h>

#include "pp_ops/fw_pp_ops.h"

// #define DEBUG
// #define DEBUG_VERBOSE
#define BAUD	57600	// Baud rate (9600 is default)

#if defined(ARDUINO_AVR_UNO)
    // Arduino UNO
    #define ISP_PORT  PORTC
    #define ISP_DDR   DDRC
    #define ISP_PIN   PINC
    #define ISP_MCLR_BIT  3     // A3 (PC3)
    #define ISP_DAT_BIT   1     // A1 (PC1)
    #define ISP_CLK_BIT   0     // A0 (PC0)
#elif defined(ARDUINO_AVR_LEONARDO)
    // Arduino Leonardo
    #define ISP_PORT  PORTF
    #define ISP_DDR   DDRF
    #define ISP_PIN   PINF
    #define ISP_MCLR_BIT  4     // A3 (PF4)
    #define ISP_DAT_BIT   6     // A1 (PF6)
    #define ISP_CLK_BIT   7     // A0 (PF7)
#else
    #error Unsupported board selection.
#endif

#define  ISP_MCLR(v)  do { if (v) ISP_PORT |=  (1<<ISP_MCLR_BIT); else ISP_PORT &= ~(1<<ISP_MCLR_BIT); } while (0)
#define  ISP_MCLR_IN  do { ISP_DDR  &= ~(1<<ISP_MCLR_BIT); } while (0)
#define  ISP_MCLR_OUT do { ISP_DDR  |=  (1<<ISP_MCLR_BIT); } while (0)

#define  ISP_DAT(v)   do { if (v) ISP_PORT |=  (1<<ISP_DAT_BIT);  else ISP_PORT &= ~(1<<ISP_DAT_BIT);  } while (0)
#define  ISP_DAT_V    (ISP_PIN&(1<<ISP_DAT_BIT))
#define  ISP_DAT_IN   do { ISP_DDR  &= ~(1<<ISP_DAT_BIT);  } while (0)
#define  ISP_DAT_OUT  do { ISP_DDR  |=  (1<<ISP_DAT_BIT);  } while (0)

#define  ISP_CLK(v)   do { if (v) ISP_PORT |=  (1<<ISP_CLK_BIT);  else ISP_PORT &= ~(1<<ISP_CLK_BIT);  } while (0)
#define  ISP_CLK_IN   do { ISP_DDR  &= ~(1<<ISP_CLK_BIT);  } while (0)
#define  ISP_CLK_OUT  do { ISP_DDR  |=  (1<<ISP_CLK_BIT);  } while (0)

#define ISP_CLK_DELAY  1
#define DELAY3 _delay_us(ISP_CLK_DELAY * 3)
#define DELAY  _delay_us(ISP_CLK_DELAY)

void isp_send (unsigned int data, unsigned char n);
unsigned int isp_read_16(void);
void acquire_isp_dat_clk(void);
void release_isp_dat_clk(void);
unsigned char enter_progmode(void);
unsigned char exit_progmode(void);
void isp_read_pgm(unsigned int * data, unsigned char n);
void isp_write_pgm(unsigned int * data, unsigned char n);
void isp_mass_erase(void);
void isp_reset_pointer(void);
void isp_send_8_msb(unsigned char data);
unsigned int isp_read_8_msb(void);
unsigned int isp_read_16_msb(void);
unsigned char p16c_enter_progmode(void);
void p16c_set_pc(unsigned long pc);
void p16c_bulk_erase(void);
void p16c_load_nvm(unsigned char inc, unsigned int data);
unsigned int p16c_read_data_nvm (unsigned char inc);
void p16c_begin_prog(unsigned char cfg_bit);
void p16c_isp_write_cfg(unsigned int data, unsigned int addr);
void p18q_isp_write_pgm(unsigned int * data, unsigned long addr, unsigned char n);
void p18q_isp_write_cfg(unsigned int data, unsigned long addr);

unsigned char p18_enter_progmode(void);
unsigned int p18_get_ID(void);
void p18_send_cmd_payload(unsigned char cmd, unsigned int payload);
unsigned int p18_get_cmd_payload(unsigned char cmd);
unsigned int isp_read_8(void);
void p18_set_tblptr(unsigned long val);
unsigned char p18_read_pgm_byte(void);
void p_18_modfied_nop(void);
void p18_isp_mass_erase(void);
void p18fk_isp_mass_erase(unsigned char data1, unsigned char data2, unsigned char data3);

void usart_tx_b(uint8_t data);
uint8_t usart_rx_rdy(void);
uint8_t usart_rx_b(void);

void exec_ops(uint8_t *ops, int n);

#if defined(DEBUG)
#define debug_print(msg ...) do { \
    char buf[100]; \
    sprintf(buf, msg); \
    Serial1.println(buf); } while (0)
#else
#define debug_print(msg ...) do { } while (0)
#endif
#if defined(DEBUG_VERBOSE)
#define verbose_print(msg ...) do { \
    char buf[100]; \
    sprintf(buf, msg); \
    Serial1.println(buf); } while (0)
#else
#define verbose_print(msg ...) do { } while (0)
#endif

int rx_state = 0;
int rx_message_ptr;
unsigned char rx_message[280];
unsigned int flash_buffer[260];

void setup(void)
{
#if defined(ARDUINO_AVR_UNO)
    uint8_t UBRR = (F_CPU/16)/BAUD - 1;	// Used for UBRRL and UBRRH
    UBRR0H = ((UBRR) & 0xF00);
    UBRR0L = (uint8_t) ((UBRR) & 0xFF);
    UCSR0B |= _BV(TXEN0);
    UCSR0B |= _BV(RXEN0);
#endif

#if defined(ARDUINO_AVR_LEONARDO)
    Serial.begin(BAUD);
    while (!Serial);
#endif

#if defined(DEBUG)
  Serial1.begin(115200);
  Serial1.println("Hello, debug serial!");
#endif

    release_isp_dat_clk();
    ISP_MCLR_OUT;
    ISP_MCLR(1);
}

void loop()
{
    int i;
    unsigned char rx_char;
    unsigned long addr;
    unsigned int cfg_val;

    if (!usart_rx_rdy())
        return;

    rx_char = usart_rx_b();
    rx_state = rx_state_machine(rx_state, rx_char);

    if (rx_state != 3)
        return;

    switch (rx_message[0]) {
    case 0x01:
        debug_print("0x01: enter_progmode");
        enter_progmode();
        usart_tx_b(0x81);
        break;
    case 0x02:
        debug_print("0x02: exit_progmode");
        exit_progmode();
        usart_tx_b(0x82);
        break;
    case 0x03:
        debug_print("0x03: isp_reset_pointer");
        isp_reset_pointer();
        usart_tx_b(0x83);
        break;
    case 0x04:
        debug_print("0x04: isp_send_config");
        isp_send_config(0);
        usart_tx_b(0x84);
        break;
    case 0x05:
        debug_print("0x05: isp_inc_pointer");
        for (i=0; i < rx_message[2]; i++) {
            isp_inc_pointer();
        }
        usart_tx_b(0x85);
        break;
    case 0x06:
        debug_print("0x06: isp_read_pgm");
        usart_tx_b(0x86);
        isp_read_pgm(flash_buffer, rx_message[2]);
        for (i=0; i < rx_message[2]; i++) {
            usart_tx_b(flash_buffer[i] & 0xFF);
            usart_tx_b(flash_buffer[i] >> 8);
        }
        break;
    case 0x07:
        debug_print("0x07: isp_mass_erase");
        isp_mass_erase();
        usart_tx_b(0x87);
        break;
    case 0x08:
        debug_print("0x08: isp_write_pgm");
        for (i=0; i < rx_message[2] / 2; i++) {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+4]))<<8) + (((unsigned int)(rx_message[(2*i)+0+4]))<<0);
        }
        isp_write_pgm(flash_buffer, rx_message[2] / 2, rx_message[3]);
        usart_tx_b(0x88);
        break;
    case 0x09:
        debug_print("0x09: isp_reset_pointer_16d");
        isp_reset_pointer_16d();
        usart_tx_b(0x89);
        break;
    case 0x10:
        debug_print("0x10: p18_enter_progmode");
        p18_enter_progmode();
        usart_tx_b(0x90);
        break;
    case 0x11:
        debug_print("0x11: p_18_isp_read_pgm");
        usart_tx_b(0x91);
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        p_18_isp_read_pgm(flash_buffer, addr, rx_message[2]);
        for (i=0; i < rx_message[2]; i++) {
            usart_tx_b(flash_buffer[i] & 0xFF);
            usart_tx_b(flash_buffer[i] >> 8);
        }
        break;
    case 0x12:
        debug_print("0x12: p18_isp_write_pgm");
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        for (i=0; i < rx_message[2] / 2; i++) {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+6]))<<8) + (((unsigned int)(rx_message[(2*i)+0+6]))<<0);
        }
        p18_isp_write_pgm(flash_buffer, addr, rx_message[2] / 2);
        usart_tx_b(0x92);
        break;
    case 0x13:
        debug_print("0x13: p18_isp_mass_erase");
        p18_isp_mass_erase();
        usart_tx_b(0x93);
        break;
    case 0x14:
        debug_print("0x14: p18_isp_write_cfg");
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        p18_isp_write_cfg(rx_message[6], rx_message[7], addr);
        usart_tx_b(0x94);
        break;
    case 0x23:
        debug_print("0x23: p18fj_isp_mass_erase");
        p18fj_isp_mass_erase();
        usart_tx_b(0xA3);
        break;
    case 0x30:
        debug_print("0x30: p18fk_isp_mass_erase");
        p18fk_isp_mass_erase(rx_message[2], rx_message[3], rx_message[4]);
        usart_tx_b(0xB0);
        break;
    case 0x31:
        debug_print("0x31: p18fk_isp_write_pgm");
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        for (i=0; i < rx_message[2] / 2; i++) {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+6]))<<8) + (((unsigned int)(rx_message[(2*i)+0+6]))<<0);
        }
        p18fk_isp_write_pgm(flash_buffer, addr, rx_message[2] / 2);
        usart_tx_b(0xB1);
        break;
    case 0x32:
        debug_print("0x32: p18fk_isp_write_cfg");
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        p18fk_isp_write_cfg(rx_message[6], rx_message[7], addr);
        usart_tx_b(0xB2);
        break;
    case 0x40:
        debug_print("0x40: p16c_enter_progmode");
        p16c_enter_progmode();
        usart_tx_b(0xC0);
        break;
    case 0x41:
        debug_print("0x41: p16c_isp_read_pgm");
        usart_tx_b(0xC1);
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        p16c_isp_read_pgm(flash_buffer, addr, rx_message[2]);
        for (i=0; i < rx_message[2]; i++) {
            usart_tx_b(flash_buffer[i] & 0xFF);
            usart_tx_b(flash_buffer[i] >> 8);
        }
        break;
    case 0x42:
        debug_print("0x42: p16c_isp_write_pgm");
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        for (i=0; i < rx_message[2] / 2; i++) {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+6]))<<8) + (((unsigned int)(rx_message[(2*i)+0+6]))<<0);
        }
        p16c_isp_write_pgm(flash_buffer, addr, rx_message[2]/2);
        usart_tx_b(0xC2);
        break;
    case 0x43:
        debug_print("0x43: p16c_bulk_erase");
        p16c_set_pc(0x8000);
        p16c_bulk_erase();
        usart_tx_b(0xC3);
        break;
    case 0x44:
        debug_print("0x44: p16c_isp_write_cfg");
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        cfg_val = rx_message[6];
        cfg_val = (cfg_val << 8) + rx_message[7];
        p16c_isp_write_cfg(cfg_val, addr);
        usart_tx_b(0xC4);
        break;
    case 0x45:
        debug_print("0x45: p18q_isp_write_cfg");
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        cfg_val = rx_message[6];
        cfg_val = (cfg_val << 8) + rx_message[7];
        p18q_isp_write_cfg(cfg_val, addr);
        usart_tx_b(0xC5);
        break;
    case 0x46:
        debug_print("0x46: p18q_isp_write_pgm");
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        for (i=0; i < rx_message[2] / 2; i++) {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+6]))<<8) + (((unsigned int)(rx_message[(2*i)+0+6]))<<0);
        }
        p18q_isp_write_pgm(flash_buffer, addr, rx_message[2]/2);
        usart_tx_b(0xC6);
        break;
    case 0x47:
        debug_print("0x47: p18q_isp_read_cfg");
        usart_tx_b(0xC7);
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        p18q_isp_read_cfg(flash_buffer, addr, rx_message[2]);
        for (i=0; i < rx_message[2]; i++) {
            usart_tx_b(flash_buffer[i] & 0xFF);
        }
        break;
    case 0x48:
        debug_print("0x48: p18q_isp_write_cfg");
        addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
        p18q_isp_write_cfg(rx_message[6], addr);
        usart_tx_b(0xC5);
        break;
    case 0x49:
        debug_print("0x49: p18qxx_bulk_erase");
        p18qxx_bulk_erase();
        usart_tx_b(0xC3);
        break;
    case 0x7f:
        usart_tx_b(0xff);
        usart_tx_b(PP_PROTO_TYPE_PPROG);
        usart_tx_b(PP_PROTO_MAJOR_VERSION);
        usart_tx_b(PP_PROTO_MINOR_VERSION);
        usart_tx_b(PP_CAP_LEGACY | PP_CAP_PP_OPS);
        break;
    case 0x80:
        debug_print("0x80: exec_ops: %02x %02x %02x %02x %02x ...",
                    rx_message[1], rx_message[2], rx_message[3], rx_message[4], rx_message[5]);
        exec_ops(&rx_message[2], rx_message[1]);
        break;
    }
    rx_state = 0;
}

unsigned char rx_state_machine(unsigned char state, unsigned char rx_char)
{
    static unsigned char bytes_to_receive = 0;

    switch (state) {
    case 0:
        rx_message_ptr = 0;
        rx_message[rx_message_ptr++] = rx_char;
        return 1;
    case 1:
        bytes_to_receive = rx_char;
        rx_message[rx_message_ptr++] = rx_char;
        if (bytes_to_receive == 0)
            return 3;
        return 2;
    case 2:
        rx_message[rx_message_ptr++] = rx_char;
        bytes_to_receive--;
        if (bytes_to_receive == 0)
            return 3;
    default:
        return state;
    }
}

void isp_read_pgm(unsigned int * data, unsigned char n)
{
    unsigned char i;
    //  DELAY3;
    for (i=0; i < n; i++) {
        isp_send(0x04, 6);
        data[i] = isp_read_14s();
        isp_send(0x06, 6);
    }
}

void isp_write_pgm(unsigned int * data, unsigned char n, unsigned char slow)
{
    unsigned char i;
    //  DELAY3;
    for (i=0; i < n; i++) {
        isp_send(0x02, 6);
        isp_send(data[i] << 1, 16);
        if (i!=(n-1))
            isp_send(0x06, 6);
    }
    isp_send(0x08, 6);
    if (slow)
        _delay_ms(5);
    else
        _delay_ms(3);
    isp_send(0x06, 6);
}

void isp_send_config(unsigned int data)
{
    isp_send(0x00, 6);
    isp_send(data, 16);
}

void isp_mass_erase(void)
{
    //_delay_ms(10);
    //  DELAY3;
    //isp_send(0x11,6);
    isp_send_config(0);
    isp_send(0x09, 6);
    _delay_ms(10);
    //isp_send(0x0B,6);
    //_delay_ms(10);
}

void isp_reset_pointer(void)
{
    //  DELAY3;
    isp_send(0x16, 6);
}

void isp_reset_pointer_16d(void)
{
    //  DELAY3;
    isp_send(0x1D, 6);
    isp_send(0x0, 8);
    isp_send(0x0, 8);
    isp_send(0x0, 8);
}

void isp_inc_pointer(void)
{
    //  DELAY3;
    isp_send(0x06, 6);
}


unsigned int isp_read_16(void)
{
    unsigned char i;
    unsigned int out;
    out = 0;
    ISP_DAT_IN;
    //  DELAY3;
    for (i=0; i < 16; i++) {
        ISP_CLK(1);
        DELAY;
        ISP_CLK(0);
        DELAY;
        out = out >> 1;
        if (ISP_DAT_V)
            out = out | 0x8000;
    }
    return out;
}

unsigned int isp_read_8(void)
{
    unsigned char i;
    unsigned int out;
    out = 0;
    ISP_DAT_IN;
    //  DELAY3;
    for (i=0; i < 8; i++) {
        ISP_CLK(1);
        DELAY;
        ISP_CLK(0);
        DELAY;
        out = out >> 1;
        if (ISP_DAT_V)
            out = out | 0x80;
    }
    return out;
}

unsigned int isp_read_14s(void)
{
    unsigned char i;
    unsigned int out;
    out = isp_read_16();
    out = out &0x7FFE;
    out = out >> 1;
    return out;
}

void isp_send(unsigned int data, unsigned char n)
{
    unsigned char i;
    ISP_DAT_OUT;
    //  DELAY3;
    for (i=0; i < n; i++) {
        ISP_DAT(data & 0x01);
        DELAY;
        ISP_CLK(1);
        //  DELAY;
        data = data >> 1;
        ISP_CLK(0);
        ISP_DAT(0);
        //  DELAY;
    }
}

void isp_send_24_msb(unsigned long data)
{
    unsigned char i;
    ISP_DAT_OUT;
    //  DELAY3;
    for (i=0; i < 23; i++) {
        ISP_DAT(data & 0x400000);
        DELAY;
        ISP_CLK(1);
        DELAY;
        data = data << 1;
        ISP_CLK(0);
        //  DELAY;
    }
    ISP_DAT(0);
    DELAY;
    ISP_CLK(1);
    DELAY;
    ISP_CLK(0);
}

void isp_send_8_msb(unsigned char data)
{
    unsigned char i;
    ISP_DAT_OUT;
    //  DELAY3;
    for (i=0; i < 8; i++) {
        ISP_DAT(data & 0x80);
        DELAY;
        ISP_CLK(1);
        DELAY;
        data = data << 1;
        ISP_CLK(0);
        ISP_DAT(0);
        //  DELAY;
    }
}

unsigned int isp_read_8_msb(void)
{
    unsigned char i;
    unsigned int out;
    out = 0;
    ISP_DAT_IN;
    //  DELAY3;
    for (i=0; i < 8; i++) {
        ISP_CLK(1);
        DELAY;
        ISP_CLK(0);
        DELAY;
        out = out << 1;
        if (ISP_DAT_V)
            out = out | 0x1;
    }
    return out;
}

unsigned int isp_read_16_msb(void)
{
    unsigned char i;
    unsigned int out;
    out = 0;
    ISP_DAT_IN;
    //  DELAY3;
    for (i=0; i < 16; i++) {
        ISP_CLK(1);
        DELAY;
        ISP_CLK(0);
        DELAY;
        out = out << 1;
        if (ISP_DAT_V)
            out = out | 0x1;
    }
    return out;
}

void acquire_isp_dat_clk(void)
{
    ISP_DAT(0);
    ISP_CLK(0);
    ISP_CLK_OUT;
    ISP_DAT_OUT;
}

void release_isp_dat_clk(void)
{
    ISP_CLK_IN;
    ISP_DAT_IN;
}

unsigned char enter_progmode(void)
{
    acquire_isp_dat_clk();
    ISP_MCLR(0);
    _delay_us(300);
    isp_send(0b01010000, 8);
    isp_send(0b01001000, 8);
    isp_send(0b01000011, 8);
    isp_send(0b01001101, 8);

    isp_send(0, 1);
}

/**************************************************************************************************************************/

unsigned char p18_enter_progmode(void)
{
    acquire_isp_dat_clk();
    ISP_MCLR(0);
    _delay_us(300);
    isp_send(0b10110010, 8);
    isp_send(0b11000010, 8);
    isp_send(0b00010010, 8);
    isp_send(0b00001010, 8);
    _delay_us(300);
    ISP_MCLR(1);
}

void p18_isp_mass_erase(void)
{
    p18_set_tblptr(0x3C0005);
    p18_send_cmd_payload(0x0C, 0x0F0F);
    p18_set_tblptr(0x3C0004);
    p18_send_cmd_payload(0x0C, 0x8F8F);
    p18_send_cmd_payload(0, 0x0000);
    isp_send(0x00, 4);
    _delay_ms(20);
    isp_send(0x00, 16);
}

void p18fj_isp_mass_erase(void)
{
    p18_set_tblptr(0x3C0005);
    p18_send_cmd_payload(0x0C, 0x0101);
    p18_set_tblptr(0x3C0004);
    p18_send_cmd_payload(0x0C, 0x8080);
    p18_send_cmd_payload(0, 0x0000);
    isp_send(0x00, 4);
    _delay_ms(600);
    isp_send(0x00, 16);
}


void p18fk_isp_mass_erase(unsigned char data1, unsigned char data2, unsigned char data3)
{
    unsigned int tmp1, tmp2, tmp3;
    tmp1 = data1;
    tmp1 = (tmp1 << 8) | data1;
    tmp2 = data2;
    tmp2 = (tmp2 << 8) | data2;
    tmp3 = data3;
    tmp3 = (tmp3 << 8) | data3;
    p18_set_tblptr(0x3C0004);
    p18_send_cmd_payload(0x0C, tmp3);
    p18_set_tblptr(0x3C0005);
    p18_send_cmd_payload(0x0C, tmp2);
    p18_set_tblptr(0x3C0006);
    p18_send_cmd_payload(0x0C, tmp1);
    p18_send_cmd_payload(0x00, 0);
    isp_send(0x00, 4);
    _delay_ms(5);
    isp_send(0x00, 16);
}

void p18fk_isp_write_pgm(unsigned int * data, unsigned long addr, unsigned char n)
{
    unsigned char i;
    //  DELAY3;
    p18_send_cmd_payload(0, 0x8E7F);
    p18_send_cmd_payload(0, 0x9C7F);
    p18_send_cmd_payload(0, 0x847F);
    p18_set_tblptr(addr);
    for (i=0; i < n-1; i++)
        p18_send_cmd_payload(0x0D, data[i]);
    p18_send_cmd_payload(0x0F, data[n - 1]);
    p_18_modfied_nop(0);
}

void p18_isp_write_pgm(unsigned int * data, unsigned long addr, unsigned char n)
{
    unsigned char i;
    //  DELAY3;
    p18_send_cmd_payload(0, 0x8EA6);
    p18_send_cmd_payload(0, 0x9CA6);
    p18_send_cmd_payload(0, 0x84A6);
    p18_set_tblptr(addr);
    for (i=0; i < n - 1; i++)
        p18_send_cmd_payload(0x0D, data[i]);
    p18_send_cmd_payload(0x0F, data[n-1]);
    p_18_modfied_nop(1);
}

void p18_isp_write_cfg(unsigned char data1, unsigned char data2, unsigned long addr)
{
    unsigned int i;
    //  DELAY3;
    p18_send_cmd_payload(0, 0x8EA6);
    p18_send_cmd_payload(0, 0x8CA6);
    p18_send_cmd_payload(0, 0x84A6);
    p18_set_tblptr(addr);
    p18_send_cmd_payload(0x0F, data1);
    p_18_modfied_nop(1);
    _delay_ms(5);
    p18_set_tblptr(addr + 1);
    i = data2;
    i = i << 8;
    p18_send_cmd_payload(0x0F, i);
    p_18_modfied_nop(1);
    _delay_ms(5);
}

void p18q_isp_read_cfg(unsigned int * data, unsigned long addr, unsigned char n)
{
    int i;
    unsigned int retval;
    unsigned char tmp;

    //  DELAY3;
    p16c_set_pc(addr);
    for (i=0; i < n; i++) {
        isp_send_8_msb(0xFE);
        _delay_us(2);
        tmp = isp_read_16_msb();
        retval = isp_read_8_msb();
        _delay_us(2);
        retval = retval >> 1;
        if (tmp & 0x01)
            retval = retval | 0x80;
        data[i] = retval;
    }
}

void p18fk_isp_write_cfg(unsigned char data1, unsigned char data2, unsigned long addr)
{
    unsigned int i;
    //  DELAY3;
    p18_send_cmd_payload(0, 0x8E7F);
    p18_send_cmd_payload(0, 0x8C7F);
    p18_set_tblptr(addr);
    p18_send_cmd_payload(0x0F, data1);
    p_18_modfied_nop(1);
    _delay_ms(5);
    p18_set_tblptr(addr + 1);
    i = data2;
    i = i << 8;
    p18_send_cmd_payload(0x0F, i);
    p_18_modfied_nop(1);
    _delay_ms(5);
}

void p_18_modfied_nop(unsigned char nop_long)
{
    unsigned char i;
    ISP_DAT_OUT;
    ISP_DAT(0);
    for (i=0; i < 3; i++) {
        DELAY;
        ISP_CLK(1);
        DELAY;
        ISP_CLK(0);
    }
    DELAY;
    ISP_CLK(1);
    if (nop_long)
        _delay_ms(4);
    _delay_ms(1);
    ISP_CLK(0);
    DELAY;
    isp_send(0x00, 16);
}

void p_18_isp_read_pgm(unsigned int * data, unsigned long addr, unsigned char n)
{
    unsigned char i;
    unsigned int tmp1, tmp2;
    //  DELAY3;
    p18_set_tblptr(addr);
    for (i=0; i < n; i++) {
        tmp1 =  p18_read_pgm_byte();
        tmp2 =  p18_read_pgm_byte();
        tmp2 = tmp2 << 8;
        data[i] = tmp1 | tmp2;
    }
}

void p18_set_tblptr(unsigned long val)
{
    p18_send_cmd_payload(0, 0x0E00 | ((val >> 16) & 0xFF));
    p18_send_cmd_payload(0, 0x6EF8);
    p18_send_cmd_payload(0, 0x0E00 | ((val >> 8) & 0xFF));
    p18_send_cmd_payload(0, 0x6EF7);
    p18_send_cmd_payload(0, 0x0E00 | ((val >> 0) & 0xFF));
    p18_send_cmd_payload(0, 0x6EF6);
}

unsigned char p18_read_pgm_byte(void)
{
    isp_send(0x09, 4);
    isp_send(0x00, 8);
    return isp_read_8();
}

unsigned int p18_get_ID(void)
{
    unsigned int temp;

    p18_set_tblptr(0x3FFFFE);
    temp = p18_read_pgm_byte();
    temp = temp << 8;
    temp = temp | p18_read_pgm_byte();
    return temp;
}

void p18_send_cmd_payload(unsigned char cmd, unsigned int payload)
{
    isp_send(cmd, 4);
    isp_send(payload, 16);
    _delay_us(30);
}

unsigned int p18_get_cmd_payload(unsigned char cmd)
{
    isp_send(cmd, 4);
    return isp_read_16();
}

unsigned char exit_progmode(void)
{
    release_isp_dat_clk();
    ISP_MCLR(1);
    _delay_ms(30);
    ISP_MCLR(0);
    _delay_ms(30);
    ISP_MCLR(1);
}

//***********************************************************************************//

unsigned char p16c_enter_progmode(void)
{
    acquire_isp_dat_clk();
    ISP_MCLR(0);
    _delay_us(300);
    isp_send_8_msb(0x4d);
    isp_send_8_msb(0x43);
    isp_send_8_msb(0x48);
    isp_send_8_msb(0x50);
    _delay_us(300);
}

void p16c_set_pc(unsigned long pc)
{
    isp_send_8_msb(0x80);
    _delay_us(2);
    isp_send_24_msb(pc);
}

void p16c_bulk_erase(void)
{
    isp_send_8_msb(0x18);
    _delay_ms(100);
}

void p16c_load_nvm(unsigned int data, unsigned char inc)
{
    if (inc==0)
        isp_send_8_msb(0x00);
    else
        isp_send_8_msb(0x02);
    _delay_us(2);
    isp_send_24_msb(data);
    _delay_us(2);
}

unsigned int p16c_read_data_nvm(unsigned char inc)
{
    unsigned int retval;
    unsigned char tmp;
    if (inc==0)
        isp_send_8_msb(0xFC);
    else
        isp_send_8_msb(0xFE);
    _delay_us(2);
    tmp = isp_read_8_msb();
    retval = isp_read_16_msb();
    retval = retval >> 1;
    if (tmp & 0x01)
        retval = retval | 0x8000;
    return retval;
}

void p16c_begin_prog(unsigned char cfg_bit)
{
    isp_send_8_msb(0xE0);
    _delay_ms(3);
    if (cfg_bit)
        _delay_ms(3);
}

unsigned int p16c_get_ID(void)
{
    p16c_set_pc(0x8006);
    return p16c_read_data_nvm(1);
}

void p16c_isp_write_pgm(unsigned int * data, unsigned long addr, unsigned char n)
{
    unsigned char i;
    //  DELAY3;
    p16c_set_pc(addr);
    for (i=0; i < n; i++)
        p16c_load_nvm(data[i], 1);
    p16c_set_pc(addr);
    p16c_begin_prog(0);
}

void p16c_isp_read_pgm(unsigned int * data, unsigned long addr, unsigned char n)
{
    unsigned char i;
    unsigned int tmp1, tmp2;
    //  DELAY3;
    p16c_set_pc(addr);
    for (i=0; i < n; i++)
        data[i] = p16c_read_data_nvm(1);
}

void p16c_isp_write_cfg(unsigned int data, unsigned long addr)
{
    unsigned char i;
    //  DELAY3;
    p16c_set_pc(addr);
    p16c_load_nvm(data,0);
    p16c_begin_prog(1);
}

void p18qxx_bulk_erase(void)
{
    isp_send_8_msb(0x18);
    _delay_us(2);
    isp_send_24_msb(0x00001e);    // Bit 3: Configuration memory Bit 1: Flash memory
    _delay_ms(11);
}

void p18q_isp_write_pgm(unsigned int * data, unsigned long addr, unsigned char n)
{
    unsigned char i;
    //  DELAY3;
    p16c_set_pc(addr);
    for (i=0; i < n; i++) {
        isp_send_8_msb(0xE0);
        _delay_us(2);
        isp_send_24_msb(data[i]);
        _delay_us(75);
    }
}

void p18q_isp_write_cfg(unsigned int data, unsigned long addr)
{
    unsigned char i;
    //  DELAY3;
    p16c_set_pc(addr);
    isp_send_8_msb(0xE0);
    _delay_us(2);
    isp_send_24_msb(data);
    _delay_ms(11);
}

#if defined(ARDUINO_AVR_UNO)
void usart_tx_b(uint8_t data)
{
    while (!(UCSR0A & _BV(UDRE0)));
    UDR0 = data;
}

uint8_t usart_rx_rdy(void)
{
    if (UCSR0A & _BV(RXC0))
        return 1;
    else
        return 0;
}

uint8_t usart_rx_b(void)
{
    return (uint8_t) UDR0;
}
#endif

#if defined(ARDUINO_AVR_LEONARDO)
void usart_tx_b(uint8_t data)
{
    Serial.write(data);
}

uint8_t usart_rx_rdy(void)
{
    return Serial.available();
}

uint8_t usart_rx_b(void)
{
    return (uint8_t)Serial.read();
}
#endif

// #define fw_pp_ops_delay(n) do { uint8_t count = (n); while (0 < count--) _delay_us(10); } while (0)
#define fw_pp_ops_delay(n) do { delayMicroseconds(n); } while (0)
// #define fw_pp_ops_clk_delay() fw_pp_ops_delay(pp_params[PP_PARAM_CLK_DELAY])
#define fw_pp_ops_clk_delay() DELAY

#include "pp_ops/fw_pp_ops.c"
