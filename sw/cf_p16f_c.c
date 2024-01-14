#include "pp3.h"
#include <stdlib.h>
#include <string.h>

int cf_p16f_c_set_pc(unsigned long pc)
{
    pp_ops_write_isp_8(0x80);
    pp_ops_delay_us(2);
    pp_ops_write_isp_24(pc);

    return 0;
}

int cf_p16f_c_enter_progmode(void)
{
    pp_ops_init();

    // acquire_isp_dat_clk();
    pp_ops_io_dat_out(0);
    pp_ops_io_clk_out(0);

    pp_ops_io_mclr(0);
    pp_ops_delay_us(300);

    uint8_t buf[] = { 0x4d, 0x43, 0x48, 0x50 };
    pp_ops_write_isp(buf, sizeof(buf));
    pp_ops_delay_us(300);
    pp_ops_reply(0xc0);

    int n;
    n = sizeof(buf);
    pp_ops_exec(buf, &n);
    debug_print("%s: n=%d, replay=0x%02x\n", __func__, n, buf[0]);

    return 0;
}

int cf_p16f_c_read_page(uint8_t *data, int address, int num)
{
    uint8_t buf[1024];
    int i, n;

    num = num / 2 * 2;
    debug_print("Reading page of %d bytes at 0x%6.6x\n", num, address);

    pp_ops_init();
    pp_ops_param_reset();
    pp_ops_param_set(PP_PARAM_CLK_DELAY, 1);
    pp_ops_param_set(PP_PARAM_CMD1, 0xfe);
    pp_ops_param_set(PP_PARAM_CMD1_LEN, 8);
    pp_ops_param_set(PP_PARAM_DELAY1, 2);
    pp_ops_param_set(PP_PARAM_PREFIX_LEN, 7);
    pp_ops_param_set(PP_PARAM_DATA_LEN, 16);
    pp_ops_param_set(PP_PARAM_POSTFIX_LEN, 1);
    pp_ops_reply(0xc1);
    cf_p16f_c_set_pc(address);
    pp_ops_read_isp_bits(num);

    n = sizeof(buf);
    pp_ops_exec(buf, &n);

    for (i = 0; i < num; i += 2) {
        *data++ = buf[i + 2];
        *data++ = buf[i + 1];
    }

    return 0;
}
