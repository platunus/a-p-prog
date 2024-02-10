#include "pp3.h"
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

char * PP_VERSION = "0.99";

int verbose = 1, verify = 1, program = 1;
int legacy_mode = 0;
int reset = 0;
// set a proper init value for sleep time to avoid a lot of issues such as 'rx fail'.
int sleep_time = 2000;
int reset_time = 30;
char *cpu_type_name = NULL;
char *comm_port_name = NULL;
char *input_file_name = NULL;
int devid_expected, devid_mask, flash_size, page_size;
unsigned char progmem[PROGMEM_LEN], config_bytes[CONFIG_LEN];
uint32_t pp_fw_caps = 0;

chip_family_t *chip_families[] = {
    &cf_p16f_a,
    &cf_p16f_b,
    &cf_p16f_c,
    &cf_p16f_d,
    &cf_p18q43,
    &cf_p18q8x,
    NULL
};
chip_family_t *cf = NULL;

int is_empty(unsigned char *buff, int len)
{
    int i,empty;
    empty = 1;
    for (i = 0; i < len; i++) {
        if (buff[i] != 0xFF) {
            empty = 0;
        }
    }
    return empty;
}

void printHelp()
{
    printf("pp programmer\n");
    printf("Usage:\n");
    printf("-c PORT : serial port device\n");
    printf("-t MODEL : target MCU model, such as '16f1824'\n");
    printf("-s TIME : sleep time in ms while arduino bootloader expires (default: 2000)\n");
    printf("-r TIME : reset target and sleep time in ms before programming (default: 30)\n");
    printf("-v NUM : verbose output level (default: 1)\n");
    printf("-n : skip verify after program\n");
    printf("-p : skip program \n");
    printf("-L : force to use legacy chip_family routines\n");
    printf("-V : Just verify withouy program\n");
    printf("-h : show this help message and exit\n");
}

void parseArgs(int argc, char *argv[])
{
    int c;
    while ((c = getopt(argc, argv, "c:npLVhs:r:t:v:")) != -1) {
        switch (c) {
        case 'c' :
            comm_port_name = optarg;
            break;
        case 'n':
            verify = 0;
            break;
        case 'p':
            program = 0;
            // skip program means also skip verify.
            verify = 0;
            break;
        case 'L':
            legacy_mode = 1;
            break;
        case 'V':
            program = 0;
            verify = 1;
            break;
        case 's' :
            sscanf(optarg,"%d",&sleep_time);
            break;
        case 'r' :
            reset = 1;
            sscanf(optarg,"%d",&reset_time);
            break;
        case 't' :
            cpu_type_name = strdup(optarg);
            break;
        case 'v' :
            sscanf(optarg,"%d",&verbose);
            break;
        case 'h' :
            printHelp();
            exit(0);
        case '?' :
            if (isprint (optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr,"Unknown option character `\\x%x'.\n",optopt);
        default:
            fprintf(stderr,"Bug, unhandled option '%c'\n",c);
            exit(1);
        }
    }
    if (argc <= 1) {
        printHelp();
        exit(1);
    }
    input_file_name = argv[argc - 1];
}

/*
 * programming routines
 */
int setCPUtype(char* cpu)
{
    int name_len, i, read;
    name_len = strlen(cpu);
    for(i = 0; i < name_len; i++)
        cpu[i] = tolower(cpu[i]);
    char * line = NULL;
    char * filename = "pp3_devices.dat";
    char read_cpu_type[20], read_algo_type[20];
    int read_flash_size, read_page_size, read_id, read_mask;

    size_t len = 0;
    debug_print("Opening filename %s \n", filename);
    FILE* sf = fopen(filename, "r");
#if defined(__linux__) || defined(__APPLE__)
    if (sf == 0) {
        filename = "/etc/pp3/pp3_devices.dat";
        sf = fopen(filename, "r");
    }
#endif
    if (sf == 0) {
        info_print("Can't open device database file '%s'\n",filename);
        exit(1);
    }
    debug_print("File open\n");
    while ((read = pp_util_getline(&line, &len, sf)) != -1) {
        dump_print("\nRead %d chars: %s",read,line);
        if (line[0] != '#') {
            sscanf(line,"%s %d %d %x %x %s", (char*)&read_cpu_type,
                    &read_flash_size,&read_page_size,&read_id,&read_mask,(char*)&read_algo_type);
            dump_print("\n*** %s,%d,%d,%x,%x,%s", read_cpu_type,
                       read_flash_size, read_page_size, read_id, read_mask, read_algo_type);
            if (strcmp(read_cpu_type,cpu) != 0)
                continue;

            flash_size = read_flash_size;
            page_size = read_page_size;
            devid_expected = read_id;
            devid_mask = read_mask;
            debug_print("Found database match %s,%d,%d,%x,%x,%s\n", read_cpu_type,
                        read_flash_size, read_page_size, read_id, read_mask, read_algo_type);
            cf = NULL;
            for (i = 0; chip_families[i] != NULL; i++) {
                if (strcmp(chip_families[i]->name, read_algo_type) == 0) {
                    cf = chip_families[i];
                    break;
                }
            }
        }
    }
    fclose(sf);

    // means specified cpu not found in 'pp3_devices.dat'
    if (flash_size==0) {
        printf("PIC model '%s' not supported, please consider to add it to 'pp3_devices.dat'\n",
               cpu);
        exit(1);
    }
    return 0;
}

/*
 * hex parse and main function
 */
int parseHex(char * filename, unsigned char * progmem, unsigned char * config)
{
    char * line = NULL;
    unsigned char line_content[128];
    size_t len = 0;
    int i,temp, read,line_len, line_type, line_address, line_address_offset,effective_address;

    debug_print("Opening filename %s \n", filename);
    FILE* sf = fopen(filename, "r");
    if (sf == 0) {
        fprintf (stderr, "Can't open hex file %s\n", filename);
        return -1;
    }
    line_address_offset = 0;

    debug_print("File open\n");
    while ((read = pp_util_getline(&line, &len, sf)) != -1) {
        dump_print("\nRead %d chars: %s",read,line);
        if (line[0] != ':') {
            info_print("--- : invalid\n");
            return -1;
        }
        sscanf(line+1, "%2X", &line_len);
        sscanf(line+3, "%4X", &line_address);
        sscanf(line+7, "%2X", &line_type);
        effective_address = line_address+(65536*line_address_offset);
        dump_print("Line len %d B, type %d, address 0x%4.4x offset 0x%4.4x, EFF 0x%6.6x\n",
                   line_len,line_type,line_address,line_address_offset,effective_address);
        if (line_type == 0) {
            for (i = 0; i < line_len; i++) {
                sscanf(line+9+i*2, "%2X", &temp);
                line_content[i] = temp;
            }
            if (effective_address < flash_size) {
                dump_print("PM ");
                for (i = 0; i < line_len; i++)
                    progmem[effective_address+i] = line_content[i];
            }
            if (cf &&
                cf->config_address <= effective_address &&
                effective_address < cf->config_address + cf->config_address) {
                dump_print("CB ");
                for (i = 0; i < line_len; i++)
                    config[effective_address - cf->config_address + i] = line_content[i];
            }
        }
        if (line_type == 4) {
            sscanf(line+9, "%4X", &line_address_offset);
        }
        for (i = 0; i < line_len; i++)
            dump_print("%2.2X", line_content[i]);
        dump_print("\n");
    }
    fclose(sf);
    return 0;
}

int checkFW(void)
{
    uint8_t type, major, minor;

    debug_print("Check firmware version and capabilities\n");
    putByte(0x7f);  // request protocol version number
    putByte(0x00);  // number of bytes remaining
    getByte();
    type = getByte();
    major = getByte();
    minor = getByte();
    pp_fw_caps = getByte();

    detail_print("FW protocol 0x%02X version %d.%d\n", type, major, minor);
    if (type != PP_PROTO_TYPE_PPROG || major != PP_PROTO_MAJOR_VERSION) {
        printf("FW protocol 0x%02X version %d.%d\n", type, major, minor);
        printf("Error, FW protocol does not match\n");
        exit(1);
    }
    if (minor < PP_PROTO_MINOR_VERSION) {
        info_print("FW protocol version %d.%d is older than version %d.%d\n", major, minor,
                   PP_PROTO_MAJOR_VERSION, PP_PROTO_MINOR_VERSION);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int i, j, pages_performed, config, econfig;
    unsigned char *pm_point, *cm_point;
    unsigned char tdat[200];
    parseArgs(argc, argv);

    if (comm_port_name == NULL) {
        printf("Please use '-c PORT' to specify correct serial device\n");
        exit(1);
    }
    info_print("PP programmer, version %s\n", PP_VERSION);

    detail_print("Opening serial port %s\n", comm_port_name);
    initSerialPort();

    for (i = 0; i < PROGMEM_LEN; i++)
        progmem[i] = 0xFF;  // assume erased memories (0xFF)
    for (i = 0; i < CONFIG_LEN; i++)
        config_bytes[i] = 0xFF;

    checkFW();
    if (legacy_mode && !(pp_fw_caps & PP_CAP_LEGACY)) {
        printf("Error, Firmware does not suppor legacy protocol\n");
        exit(1);
    }
    if (!legacy_mode)
        setCPUtype(cpu_type_name);
    if (cf == NULL) {
        if (!(pp_fw_caps & PP_CAP_LEGACY)) {
            printf("Error, Unsupported CPU type %s\n", cpu_type_name);
            exit(1);
        }
        info_print("Fall back to the legacy chip_family routines\n");
        legacy_pp3();
        // no return
    }

    if (sleep_time > 0) {
        info_print("Sleeping for %d ms while arduino bootloader expires\n", sleep_time);
        fflush(stdout);
        sleep_ms(sleep_time);
    }

    pm_point = (unsigned char *)(&progmem);
    cm_point = (unsigned char *)(&config_bytes);
    if ((program || verify) && parseHex(input_file_name, pm_point, cm_point)) {
        // parse and write content of hex file into buffers
        fprintf(stderr,"Failed to read input file.\n");
        exit(1);
    }

    if (reset) {
        printf("Reset the target and wait for %d ms\n", reset_time);
        cf->reset_target();
        sleep_ms(reset_time);
    }

    cf->enter_progmode();  // enter programming mode and probe the target
    i = cf->get_device_id();
    if (i == devid_expected) {
        info_print("Device ID: %4.4x \n", i);
    } else {
        printf("Wrong device ID: %4.4x, expected: %4.4x\n", i, devid_expected);
        printf("Check for connection to target MCU, exiting now\n");
        cf->exit_progmode();
        exit(1);
    }

    if (program) {
        pages_performed = 0;
        cf->mass_erase();
        info_print("Programming FLASH (%d B in %d pages per %d bytes): \n", flash_size,
                   flash_size / page_size, page_size);
        fflush(stdout);
        if (cf->reset_pointer)
            cf->reset_pointer();
        for (i = 0; i < flash_size; i += page_size) {
            if (!is_empty(progmem + i, page_size)) {
                cf->write_program(progmem + i, i, page_size);
                pages_performed++;
                info_print("#");
            } else {
                detail_print(".");
                if (cf->increase_pointer)
                    cf->increase_pointer(page_size);
            }
        }

        info_print("\n%d pages programmed\n",pages_performed);
        info_print("Programming config\n");
        if (verbose > 2) {  // equivalent to debug_print() condition
            pp_util_hexdump("Write config: ", 0, config_bytes, cf->config_size);
        }
        cf->write_config(config_bytes, cf->config_size);
    }
    if (verify) {
        pages_performed = 0;
        info_print("Verifying FLASH (%d B in %d pages per %d bytes): \n",flash_size,
                   flash_size/page_size,page_size);
        if (cf->reset_pointer)
            cf->reset_pointer();
        for (i = 0; i < flash_size; i = i + page_size) {
            if (is_empty(progmem+i,page_size)) {
                detail_print(".");
                if (cf->increase_pointer)
                    cf->increase_pointer(page_size);
            } else {
                cf->read_program(tdat, i, page_size);
                pages_performed++;
                verbose_print("Verifying page at 0x%4.4X\n", i);
                info_print("#");
                for (j = 0; j < page_size; j++) {
                    uint8_t mask = (j % 2) ? ~cf->odd_mask : ~cf->even_mask;
                    if ((progmem[i + j] & mask) != (tdat[j] & mask)) {
                        printf("\nError at program address 0x%06X E:0x%02X R:0x%02X\n", i + j,
                               progmem[i + j], tdat[j]);
                        if (verbose > 2) {  // equivalent to debug_print() condition
                            pp_util_hexdump("      progmem: ", i + (j & ~0xf),
                                            &progmem[i + (j & ~0xf)], 16);
                            pp_util_hexdump(" Read program: ", i + (j & ~0xf),
                                            &tdat[(j & ~0xf)], 16);
                        }
                        printf("Exiting now\n");
                        cf->exit_progmode();
                        exit(1);
                    }
                }
            }
        }
        info_print("\n%d pages verified\n", pages_performed);

        info_print("Verifying config...");
        cf->read_config(tdat, cf->config_size);
        for (i = 0; i < cf->config_size; i++) {
            uint8_t mask = (i % 2) ? ~cf->odd_mask : ~cf->even_mask;
            if ((config_bytes[i] & mask) != (tdat[i] & mask)) {
                printf("Error at config address 0x%02X E:0x%02X R:0x%02X\n",
                       i, config_bytes[i], tdat[i]);
                if (verbose > 2) {  // equivalent to debug_print() condition
                    pp_util_hexdump("config_bytes: ", 0, config_bytes, cf->config_size);
                    pp_util_hexdump(" Read config: ", 0, tdat, cf->config_size);
                }
                printf("Exiting now\n");
                cf->exit_progmode();
                exit(1);
            }
        }
        info_print("OK\n");
    }

    cf->exit_progmode();
    exit(0);
}
