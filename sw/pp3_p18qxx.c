#include "pp3.h"

#define CONFIG_ADDRESS 0x300000

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

static void p16c_set_pc(unsigned long pc)
{
    pp_ops_write_isp_8(0x80);
    pp_ops_delay_us(2);
    pp_ops_write_isp_24(pc);
}

static int p16c_enter_progmode(void)
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

static int exit_progmode(void)
{
    debug_print( "Exiting programming mode\n");

    pp_ops_init();

    // release_isp_dat_clk();
    pp_ops_io_dat_in();
    pp_ops_io_clk_in();

    pp_ops_io_mclr(1);
    pp_ops_delay_ms(30);
    pp_ops_io_mclr(0);
    pp_ops_delay_ms(30);
    pp_ops_io_mclr(1);
    pp_ops_reply(0x82);

    int n;
    uint8_t buf[1];
    n = sizeof(buf);
    pp_ops_exec(buf, &n);
    debug_print("%s: n=%d, replay=0x%02x\n", __func__, n, buf[0]);

    return 0;
}

static int p18qxx_mass_erase(void)
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

static int p16c_read_page(uint8_t *data, int address, int num)
{
    uint8_t *p, buf[1024];
    int i, n;

    debug_print("Reading page of %d bytes at 0x%6.6x\n", num, address);
    num /= 2;

    int progress = 0;
    while (progress < num) {
        int chunk_size = MIN(num, 32);

        pp_ops_init();
        pp_ops_reply(0xc1);
        p16c_set_pc(address + progress * 2);

        for (i = 0; i < chunk_size; i++) {
            pp_ops_write_isp_8(0xfe);  // increment
            //pp_ops_delay_us(2);
            pp_ops_read_isp(3);
        }

        n = sizeof(buf);
        pp_ops_exec(buf, &n);

        p = &buf[1];
        for (i = 0; i < chunk_size; i++) {
            uint32_t t = ((uint32_t)p[0] << 15) | ((uint32_t)(p[1] << 7) | (p[2] >> 1));
            p += 3;
            *data++ = (t >> 0) & 0xff;
            *data++ = (t >> 8) & 0xff;
        }

        progress += chunk_size;
    }

    return 0;
}

static int p18q_write_page(uint8_t *data, int address, int num)
{
    uint8_t *p, buf[1024];
    int i, n;

    debug_print("Writing A page of %d bytes at 0x%6.6x\n", num, address);

    int empty = 1;
    for (i = 0; i < num; i = i + 2) {
        if	((data[i]!=0xFF)|(data[i+1]!=0xFF))
            empty = 0;
    }
    if (empty) {
        verbose_print("~");
        return 0;
    }


    pp_ops_init();
    p16c_set_pc(address);
    for (i = 0; i < num; i += 2) {
        uint32_t t = ((uint32_t)data[i + 1] << 8) | ((uint32_t)data[i + 0] << 0);
        pp_ops_write_isp_8(0xe0);
        //pp_ops_delay_us(2);
        pp_ops_write_isp_24(t);
        pp_ops_delay_us(75);
        if ((i % 32) == 30) {
            pp_ops_reply(0xc6);
            n = sizeof(buf);
            pp_ops_exec(buf, &n);
            pp_ops_init();
        }
    }
    if ((i % 32) != 0) {
        pp_ops_reply(0xc6);
        n = sizeof(buf);
        pp_ops_exec(buf, &n);
    }

    return 0;
}

static int p18q_read_config(uint8_t *data, int num)
{
    uint8_t *p, buf[1024];
    int i, n;

    int address = CONFIG_ADDRESS;
    debug_print( "Reading config of %d bytes at 0x%6.6x\n", num, address);

    pp_ops_init();
    pp_ops_reply(0xc7);
    p16c_set_pc(address);
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

static int p18q_write_byte_cfg(uint8_t data, int address)
{
    uint8_t buf[1];
    int n;

    pp_ops_init();
    p16c_set_pc(address);
    pp_ops_write_isp_8(0xe0);
    pp_ops_delay_us(2);
    pp_ops_write_isp_24(data);
    pp_ops_delay_ms(11);
    pp_ops_reply(0xc5);

    n = sizeof(buf);
    pp_ops_exec(buf, &n);

    return 0;
}

static int p18q_write_config(uint8_t *data, int size)
{
    for (int i = 0; i < config_size; i++) {
        p18q_write_byte_cfg(config_bytes[i], CONFIG_ADDRESS + i);
    }
    return 0;
}

static int p18q_get_device_id(void)
{
    uint8_t buf[2];
    unsigned int devid;

    p16c_read_page(buf, 0x3FFFFE, sizeof(buf));
    devid = (((unsigned int)(buf[1]))<<8) + (((unsigned int)(buf[0]))<<0);
    devid = devid & devid_mask;

    return devid;
}

chip_family_t cf_p18q43 = {
    .name = "CF_P18F_Q43",
    .id = CF_P18F_Qxx,
    .config_address = CONFIG_ADDRESS,
    .config_size = 10,
    .enter_progmode = p16c_enter_progmode,
    .exit_progmode = exit_progmode,
    .mass_erase = p18qxx_mass_erase,
    .read_page = p16c_read_page,
    .write_page = p18q_write_page,
    .read_config = p18q_read_config,
    .write_config = p18q_write_config,
    .get_device_id = p18q_get_device_id,
};

chip_family_t cf_p18q8x = {
    .name = "CF_P18F_Q8x",
    .id = CF_P18F_Qxx,
    .config_address = CONFIG_ADDRESS,
    .config_size = 35,
    .enter_progmode = p16c_enter_progmode,
    .exit_progmode = exit_progmode,
    .mass_erase = p18qxx_mass_erase,
    .read_page = p16c_read_page,
    .write_page = p18q_write_page,
    .read_config = p18q_read_config,
    .write_config = p18q_write_config,
    .get_device_id = p18q_get_device_id,
};
