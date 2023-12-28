/*
 * a-p-prog FW for PIC 16F14x
 */
#include <common.h>
#include "../fw/pp/fw_pp_ops.h"

#include <stdint.h>
#include <string.h>

#define ISP_MCLR(v)     do { LAT(FW_PP_ISP_MCLR) = (v) ? 1 : 0; } while(0)
#define ISP_MCLR_IN     do { TRIS(FW_PP_ISP_MCLR) = 1;  } while(0)
#define ISP_MCLR_OUT    do { TRIS(FW_PP_ISP_MCLR) = 0;  } while(0)

#define ISP_DAT(v)      do { LAT(FW_PP_ISP_DAT) = (v) ? 1 : 0; } while(0)
#define ISP_DAT_V       PORT(FW_PP_ISP_DAT)
#define ISP_DAT_IN      do { TRIS(FW_PP_ISP_DAT) = 1;  } while(0)
#define ISP_DAT_OUT     do { TRIS(FW_PP_ISP_DAT) = 0;  } while(0)

#define ISP_CLK(v)      do { LAT(FW_PP_ISP_CLK) = ((v) ? 1 : 0); } while(0)
#define ISP_CLK_IN      do { TRIS(FW_PP_ISP_CLK) = 1; } while(0)
#define ISP_CLK_OUT     do { TRIS(FW_PP_ISP_CLK) = 0; } while(0)

#define fw_pp_ops_delay(n)    do { \
        uint8_t count = (n); \
        while (0 < count--) \
            __delay_us(1); \
    } while(0)
#define fw_pp_ops_clk_delay() do { __delay_us(1); } while(0)

#define usart_tx_b(d) do { \
        sendback(d); \
    } while(0)

#define verbose_print(fmt, ...) do { } while(0)

static uint8_t pp_buf[280];
static void (*sendback)(uint8_t data);

void exec_ops(uint8_t *ops, int len);

void pp_init(void (*sendbackp)(uint8_t data))
{
    sendback = sendbackp;

    ISP_CLK_IN;
    ISP_DAT_IN;
    ISP_MCLR_OUT;
    ISP_MCLR(1);
}

void pp_process(const uint8_t *data, size_t len)
{
    static unsigned char bytes_to_receive = 0;
    static int state = 0;
    static int buf_len;

    while (0 < len--) {
        switch (state) {
        case 0:
            pp_buf[0] = *data++;
            if (pp_buf[0] == OP_NONE)
                continue;
            buf_len = 1;
            state = 1;
            break;
        case 1:
            bytes_to_receive = *data++;
            pp_buf[buf_len++] = bytes_to_receive;
            state = (bytes_to_receive == 0) ? 3 : 2;
            break;
        case 2:
            pp_buf[buf_len++] = *data++;
            bytes_to_receive--;
            if (bytes_to_receive == 0)
                state = 3;
            break;
        default:
            break;
        }
        if (state == 3) {
            state = 0;
            switch (pp_buf[0]) {
            case 0x80:
                exec_ops(&pp_buf[2], pp_buf[1]);
                break;
            }
        }
    }
}

#include "../fw/pp/fw_pp_ops.c"
