#include "pp3.h"
#include <stdlib.h>
#include <string.h>

int cf_p16f_b_read_config(uint8_t *data, int num)
{
    memcpy(data, config_bytes, num);

    cf_p16f_a_reset_pointer();
    cf_p16f_a_send_config(0);
    cf_p16f_a_increase_pointer(7 * 2);
    cf_p16f_a_read_page(&data[7 * 2], 0, num - (7 * 2));

    debug_print("%s: num=%d\n", __func__, num);

    return 0;
}

int cf_p16f_b_write_config(uint8_t *data, int size)
{
    debug_print("%s: num=%d\n", __func__, size);

    cf_p16f_a_reset_pointer();
    cf_p16f_a_send_config(0);
    cf_p16f_a_increase_pointer(7 * 2);
    cf_p16f_a_write_page(&config_bytes[7 * 2], 0, 2);
    cf_p16f_a_write_page(&config_bytes[8 * 2], 0, 2);
    cf_p16f_a_write_page(&config_bytes[9 * 2], 0, 2);

    return 0;
}

chip_family_t cf_p16f_b = {
    .name = "CF_P16F_B",
    .id = CF_P16F_B,
    .config_address = 0x10000,
    .config_size = 20,
    .odd_mask = 0xc0,
    .enter_progmode     = cf_p16f_a_enter_progmode,
    .exit_progmode      = cf_p16f_a_exit_progmode,
    .reset_target       = cf_p16f_a_reset_target,
    .mass_erase         = cf_p16f_a_mass_erase,
    .reset_pointer      = cf_p16f_a_reset_pointer,
    .increase_pointer   = cf_p16f_a_increase_pointer,
    .read_program       = cf_p16f_a_read_page,
    .write_program      = cf_p16f_a_write_page,
    .read_config        = cf_p16f_b_read_config,
    .write_config       = cf_p16f_b_write_config,
    .get_device_id      = cf_p16f_a_get_devid,
};
