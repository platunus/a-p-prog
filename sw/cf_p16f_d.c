#include "pp3.h"
#include <stdlib.h>
#include <string.h>

int cf_p16f_d_reset_pointer(void)
{
    uint8_t buf[300];

    debug_print("Resetting PC\n");

    pp_ops_init();
    pp_ops(isp_send(0x1d, 6));
    pp_ops(isp_send(0x00, 8));
    pp_ops(isp_send(0x00, 8));
    pp_ops(isp_send(0x00, 8));
    pp_ops(reply(0x83));

    int n = sizeof(buf);
    pp_ops(exec(buf, &n));
    debug_print("%s: n=%d, reply=0x%02x\n", __func__, n, buf[0]);

    return 0;
}

int cf_p16f_d_write_config(uint8_t *data, int size)
{
    debug_print("%s: num=%d\n", __func__, size);

    cf_p16f_a_reset_pointer();
    cf_p16f_a_send_config(0);
    cf_p16f_a_increase_pointer(7 * 2);
    cf_p16f_a_write_page(&config_bytes[0x07 * 2], 0, 2);
    cf_p16f_a_write_page(&config_bytes[0x08 * 2], 0, 2);
    cf_p16f_a_write_page(&config_bytes[0x09 * 2], 0, 2);
    cf_p16f_a_write_page(&config_bytes[0x0a * 2], 0, 2);

    return 0;
}

chip_family_t cf_p16f_d = {
    .name = "CF_P16F_D",
    .id = CF_P16F_D,
    .config_address = 0x10000,
    .config_size = 22,
    .odd_mask = 0xc0,
    .enter_progmode     = cf_p16f_a_enter_progmode,
    .exit_progmode      = cf_p16f_a_exit_progmode,
    .reset_target       = cf_p16f_a_reset_target,
    .mass_erase         = cf_p16f_a_mass_erase,
    .reset_pointer      = cf_p16f_d_reset_pointer,
    .increase_pointer   = cf_p16f_a_increase_pointer,
    .read_program       = cf_p16f_a_read_page,
    .write_program      = cf_p16f_a_write_page,
    .read_config        = cf_p16f_a_read_config,
    .write_config       = cf_p16f_d_write_config,
    .get_device_id      = cf_p16f_a_get_devid,
};
