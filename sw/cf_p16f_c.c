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

int cf_p16f_c_mass_erase(void)
{
    uint8_t buf[1];

    debug_print("Mass erase\n");

    pp_ops_init();
    cf_p16f_c_set_pc(0x8000);
    pp_ops(isp_send_8_msb(0x18));
    pp_ops(delay_ms(100));
    pp_ops(reply(0xc3));

    int n = sizeof(buf);
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

int cf_p16f_c_read_program(uint8_t *data, int address, int num)
{
    return cf_p16f_c_read_page(data, address / 2, num);
}

int cf_p16f_c_write_program(uint8_t *data, int address, int num)
{
    uint8_t buf[300];

    debug_print("Writing A page of %d bytes at 0x%6.6x\n", num, address);

    pp_ops_init();

    pp_ops_param_reset();
    pp_ops_param_set(PP_PARAM_CLK_DELAY, 1);
    pp_ops_param_set(PP_PARAM_CMD1, 0x02);  // increment address
    pp_ops_param_set(PP_PARAM_CMD1_LEN, 8);
    pp_ops_param_set(PP_PARAM_DELAY1, 2);
    pp_ops_param_set(PP_PARAM_PREFIX_LEN, 7);
    pp_ops_param_set(PP_PARAM_DATA_LEN, 16);
    pp_ops_param_set(PP_PARAM_POSTFIX_LEN, 1);
    pp_ops_param_set(PP_PARAM_DELAY2, 2);

    cf_p16f_c_set_pc(address / 2);
    for (int i = 0; i < num; i += 2) {
        buf[i + 0] = data[i + 1];
        buf[i + 1] = data[i + 0];
    }
    pp_ops(write_isp_bits(buf, num));

    cf_p16f_c_set_pc(address / 2);
    pp_ops(isp_send_8_msb(0xe0));
    pp_ops(delay_ms(3));
    pp_ops(reply(0xc2));

    int n = sizeof(buf);
    pp_ops_exec(buf, &n);
    debug_print("%s: n=%d, replay=0x%02x\n", __func__, n, buf[0]);

    return 0;
}

int cf_p16f_c_read_config(uint8_t *data, int num)
{
    memcpy(data, config_bytes, num);

    cf_p16f_c_read_page(&data[7 * 2], 0x8007, num - (7 * 2));

    debug_print("%s: num=%d\n", __func__, num);

    return 0;
}

int cf_p16f_c_write_config_word(uint8_t data0, uint8_t data1, int address)
{
    uint8_t buf[300];

    pp_ops_init();

    cf_p16f_c_set_pc(address);
    pp_ops(isp_send_8_msb(0x00));
    pp_ops(delay_us(2));
    pp_ops(isp_send_24_msb((((uint16_t)data0) << 0) + (((uint16_t)data1) << 8)));
    pp_ops(delay_us(2));
    pp_ops(isp_send_8_msb(0xe0));
    pp_ops(delay_ms(6));
    pp_ops(reply(0xc4));

    int n = sizeof(buf);
    pp_ops_exec(buf, &n);
    debug_print("%s: n=%d, replay=0x%02x\n", __func__, n, buf[0]);

    return 0;
}

int cf_p16f_c_write_config(uint8_t *data, int size)
{
    cf_p16f_c_write_config_word(data[0x07 * 2 + 0], data[0x07 * 2 + 1], 0x8007);
    cf_p16f_c_write_config_word(data[0x08 * 2 + 0], data[0x08 * 2 + 1], 0x8008);
    cf_p16f_c_write_config_word(data[0x09 * 2 + 0], data[0x09 * 2 + 1], 0x8009);
    cf_p16f_c_write_config_word(data[0x0a * 2 + 0], data[0x0a * 2 + 1], 0x800a);
    cf_p16f_c_write_config_word(data[0x0b * 2 + 0], data[0x0b * 2 + 1], 0x800b);
    return 0;
}

int cf_p16f_c_get_device_id(void)
{
    uint8_t buf[2];
    unsigned int devid;

    cf_p16f_c_read_page(buf, 0x8006, 2);
    devid = (((unsigned int)(buf[1]))<<8) + (((unsigned int)(buf[0]))<<0);
    devid &= devid_mask;

    return devid;
}

chip_family_t cf_p16f_c = {
    .name = "CF_P16F_C",
    .id = CF_P16F_C,
    .config_address = 0x10000,
    .config_size = 24,
    .odd_mask = 0xc0,
    .enter_progmode     = cf_p16f_c_enter_progmode,
    .exit_progmode      = cf_p16f_a_exit_progmode,
    .reset_target       = cf_p16f_a_reset_target,
    .mass_erase         = cf_p16f_c_mass_erase,
    .read_program       = cf_p16f_c_read_program,
    .write_program      = cf_p16f_c_write_program,
    .read_config        = cf_p16f_c_read_config,
    .write_config       = cf_p16f_c_write_config,
    .get_device_id      = cf_p16f_c_get_device_id,
};
