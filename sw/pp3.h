#include <stdio.h>
#include <stdint.h>

#define	PROGMEM_LEN	260000
#define	CONFIG_LEN	35

#define	CF_P16F_A	0
#define	CF_P18F_A	1
#define	CF_P16F_B	2
#define	CF_P18F_B	3
#define	CF_P18F_C	4
#define	CF_P18F_D	5
#define	CF_P18F_E	6
#define	CF_P16F_C	7
#define	CF_P16F_D	8
#define	CF_P18F_F	9
#define	CF_P18F_G	10
#define	CF_P18F_Q	11
#if 0
#define	CF_P18F_Q43	12
#define	CF_P18F_Q8x	13
#endif
#define	CF_P18F_Qxx	14

typedef struct {
    char *name;
    int id;
    uint32_t config_address;
    int config_size;
    int (*enter_progmode)(void);
    int (*exit_progmode)(void);
    int (*mass_erase)(void);
    int (*read_page)(uint8_t *data, int address, int num);
    int (*write_page)(uint8_t *data, int address, int num);
    int (*read_config)(uint8_t *data, int size);
    int (*write_config)(uint8_t *data, int size);
    int (*get_device_id)(void);
} chip_family_t;

extern int verbose;
extern int devid_mask, flash_size, page_size, chip_family, config_size;
extern unsigned char file_image[70000], progmem[PROGMEM_LEN], config_bytes[CONFIG_LEN];

extern chip_family_t cf_p18q43;
extern chip_family_t cf_p18q8x;

extern void sleep_us(int num);
extern void putByte(int byte);
extern int getByte(void);
extern void flsprintf(FILE* f, char *fmt, ...);

extern void pp_ops_init(void);
extern int pp_ops_io_mclr(int v);
extern int pp_ops_io_dat_in(void);
extern int pp_ops_io_dat_out(int v);
extern int pp_ops_io_clk_in(void);
extern int pp_ops_io_clk_out(int v);
extern int pp_ops_read_isp(int n);
extern int pp_ops_write_isp(uint8_t *v, int n);
extern int pp_ops_delay_us(int n);
extern int pp_ops_delay_ms(int n);
extern int pp_ops_reply(uint8_t v);
extern int pp_ops_exec(uint8_t *v, int *n);
extern int pp_ops_write_isp_8(uint8_t v);
extern int pp_ops_write_isp_24(uint32_t v);

#define info_print(fmt ...) do { if (verbose > 0) \
            flsprintf(stdout, fmt); } while (0)
#define debug_print(fmt ...) do { if (verbose > 2) \
            flsprintf(stdout, fmt); } while (0)
#define verbose_print(fmt ...) do { if (verbose > 3) \
            flsprintf(stdout, fmt); } while (0)
#define dump_print(fmt ...) do { if (verbose > 4) \
            flsprintf(stdout, fmt); } while (0)
