#include "pp3.h"
#include <stdlib.h>

#define CONFIG_ADDRESS 0x300000

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int cf_p18f_qxx_mass_erase(void)
{
    debug_print( "Mass erase\n");
    pp_ops_init();

    uint8_t buf[] = { 0x18, 0x00, 0x00, 0x1e };
    pp_ops_write_isp(&buf[0], 1);
    pp_ops_delay_us(2);
    pp_ops_write_isp_24(0x00001e);
    pp_ops_delay_ms(11);

    pp_ops_reply(0xc3);

    int n;
    n = sizeof(buf);
    pp_ops_exec(buf, &n);
    debug_print("%s: n=%d, replay=0x%02x\n", __func__, n, buf[0]);
    return 0;
}

int cf_p18f_q_write_page(uint8_t *data, int address, int num)
{
    uint8_t buf[1024];
    int i, n;

    debug_print("Writing A page of %d bytes at 0x%6.6x\n", num, address);

    pp_ops_init();
    pp_ops_param_reset();
    pp_ops_param_set(PP_PARAM_CLK_DELAY, 1);
    pp_ops_param_set(PP_PARAM_CMD1, 0xe0);
    pp_ops_param_set(PP_PARAM_CMD1_LEN, 8);
    pp_ops_param_set(PP_PARAM_DELAY1, 2);
    pp_ops_param_set(PP_PARAM_PREFIX_LEN, 7);
    pp_ops_param_set(PP_PARAM_DATA_LEN, 16);
    pp_ops_param_set(PP_PARAM_POSTFIX_LEN, 1);
    pp_ops_param_set(PP_PARAM_DELAY3, 75);
    for (i = 0; i < num; i += 2) {
        buf[i + 0] = data[i + 1];
        buf[i + 1] = data[i + 0];
    }
    cf_p16f_c_set_pc(address);
    pp_ops_write_isp_bits(buf, num);
    if (!(pp_fw_caps & PP_CAP_ASYNC_WRITE)) {
        pp_ops_reply(0xc6);  // Remove waiting for completion to speed up
    }

    n = sizeof(buf);
    pp_ops_exec(buf, &n);

    return 0;
}

int cf_p18f_q_read_config(uint8_t *data, int num)
{
    uint8_t *p, buf[1024];
    int i, n;

    int address = CONFIG_ADDRESS;
    debug_print( "Reading config of %d bytes at 0x%6.6x\n", num, address);

    pp_ops_init();
    pp_ops_reply(0xc7);
    cf_p16f_c_set_pc(address);
    for (i=0; i < num; i++) {
        pp_ops_write_isp_8(0xfe);
        pp_ops_delay_us(2);
        pp_ops_read_isp(3);
        pp_ops_delay_us(2);
    }
    n = sizeof(buf);
    pp_ops_exec(buf, &n);

    p = &buf[1];
    for (i = 0; i < num; i++) {
        uint32_t t = ((uint32_t)p[0] << 15) | ((uint32_t)(p[1] << 7) | (p[2] >> 1));
        p += 3;
        *data++ = t & 0xff;
    }

    return 0;
}

static int cf_p18f_q_write_byte_cfg(uint8_t data, int address)
{
    uint8_t buf[1];
    int n;

    pp_ops_init();
    cf_p16f_c_set_pc(address);
    pp_ops_write_isp_8(0xe0);
    pp_ops_delay_us(2);
    pp_ops_write_isp_24(data);
    pp_ops_delay_ms(11);
    pp_ops_reply(0xc5);

    n = sizeof(buf);
    pp_ops_exec(buf, &n);

    return 0;
}

int cf_p18f_q_write_config(uint8_t *data, int size)
{
    for (int i = 0; i < size; i++) {
        cf_p18f_q_write_byte_cfg(config_bytes[i], CONFIG_ADDRESS + i);
    }
    return 0;
}

int cf_p18f_q_get_device_id(void)
{
    uint8_t buf[2];
    unsigned int devid;

    cf_p16f_c_read_page(buf, 0x3FFFFE, sizeof(buf));
    devid = (((unsigned int)(buf[1]))<<8) + (((unsigned int)(buf[0]))<<0);
    devid = devid & devid_mask;

    return devid;
}

chip_family_t cf_p18q43 = {
    .name = "CF_P18F_Q43",
    .id = CF_P18F_Qxx,
    .config_address = CONFIG_ADDRESS,
    .config_size = 10,
    .enter_progmode     = cf_p16f_c_enter_progmode,
    .exit_progmode      = cf_p16f_a_exit_progmode,
    .reset_target       = cf_p16f_a_reset_target,
    .mass_erase         = cf_p18f_qxx_mass_erase,
    .read_program       = cf_p16f_c_read_page,
    .write_program      = cf_p18f_q_write_page,
    .read_config        = cf_p18f_q_read_config,
    .write_config       = cf_p18f_q_write_config,
    .get_device_id      = cf_p18f_q_get_device_id,
};

chip_family_t cf_p18q8x = {
    .name = "CF_P18F_Q8x",
    .id = CF_P18F_Qxx,
    .config_address = CONFIG_ADDRESS,
    .config_size = 35,
    .enter_progmode     = cf_p16f_c_enter_progmode,
    .exit_progmode      = cf_p16f_a_exit_progmode,
    .reset_target       = cf_p16f_a_reset_target,
    .mass_erase         = cf_p18f_qxx_mass_erase,
    .read_program       = cf_p16f_c_read_page,
    .write_program      = cf_p18f_q_write_page,
    .read_config        = cf_p18f_q_read_config,
    .write_config       = cf_p18f_q_write_config,
    .get_device_id      = cf_p18f_q_get_device_id,
};
