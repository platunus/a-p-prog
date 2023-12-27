#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>


#if defined(__linux__) || defined(__APPLE__)
#include <termios.h>
#else
#include <windows.h>
#endif

#include "pp3.h"

int setCPUtype(char* cpu);
int parse_hex (char * filename, unsigned char * progmem, unsigned char * config);
size_t getlinex(char **lineptr, size_t *n, FILE *stream) ;
void comErr(char *fmt, ...);

char * COM = "";
char * PP_VERSION = "0.99";

int verbose = 1, verify = 1, program = 1;
int reset = 0;
// set a proper init value for sleep time to avoid a lot of issues such as 'rx fail'.
int sleep_time = 2000;
int reset_time = 30;
int devid_expected, devid_mask, baudRate, com, flash_size, page_size, chip_family, config_size;
unsigned char file_image[70000], progmem[PROGMEM_LEN], config_bytes[CONFIG_LEN];

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

chip_family_t *chip_families[] = {
    &cf_p18q43,
    &cf_p18q8x,
    NULL
};
chip_family_t *cf = NULL;

/*
 * serial IO interfaces for Linux and windows
 */
#if defined(__linux__) || defined(__APPLE__)

void initSerialPort()
{
    baudRate = B57600;
    debug_print("Opening: %s at %d\n", COM, baudRate);
    com =  open(COM, O_RDWR | O_NOCTTY | O_NDELAY);
    if (com < 0)
        comErr("Failed to open serial port\n");

    struct termios opts;
    memset (&opts, 0, sizeof(opts));

    fcntl(com, F_SETFL, 0);
    if (tcgetattr(com, &opts) != 0)
        printf("Err tcgetattr\n");

    cfsetispeed(&opts, baudRate);
    cfsetospeed(&opts, baudRate);
    opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    opts.c_cflag |= (CLOCAL | CREAD);
    opts.c_cflag &= ~PARENB;
    opts.c_cflag &= ~CSTOPB;
    opts.c_cflag &= ~CSIZE;
    opts.c_cflag |= CS8;
    opts.c_oflag &= ~OPOST;
    opts.c_iflag &= ~INPCK;
    opts.c_iflag &= ~ICRNL;  // do NOT translate CR to NL
    opts.c_iflag &= ~(IXON | IXOFF | IXANY);
    opts.c_cc[VMIN] = 0;
    opts.c_cc[VTIME] = 10;  // 0.1 sec
    if (tcsetattr(com, TCSANOW, &opts) != 0) {
        perror(COM);
        printf("set attr error");
        abort();
    }
    tcflush(com, TCIOFLUSH);  // just in case some crap is the buffers
}

void putByte(int byte)
{
    char buf = byte;
    verbose_print("TX: 0x%02X\n", byte);
    int n = write(com, &buf, 1);
    if (n != 1) comErr("Serial port failed to send a byte, write returned %d\n", n);
}

void putBytes(unsigned char * data, int len)
{
    int i;
    for (i = 0; i < len; i++)
        putByte(data[i]);
    /*
    verbose_print("TXP: %d B\n", len);
    int n = write(com, data, len);
    if (n != len)
        comErr("Serial port failed to send %d bytes, write returned %d\n", len, n);
    */
}

int getByte()
{
    char buf;
    int n = read(com, &buf, 1);
    verbose_print(n < 1 ? "RX: fail\n" : "RX:  0x%02X\n", buf & 0xFF);
    if (n == 1)
        return buf & 0xFF;

    comErr("Serial port failed to receive a byte, read returned %d\n", n);
    return -1;  // never reached
}

#else  // defined(__linux__) || defined(__APPLE__)

HANDLE port_handle;

void initSerialPort()
{
    char mode[40], portname[20];
    COMMTIMEOUTS timeout_sets;
    DCB port_sets;
    strcpy(portname, "\\\\.\\");
    strcat(portname, COM);
    port_handle = CreateFileA(portname,
                              GENERIC_READ|GENERIC_WRITE,
                              0,                          /* no share  */
                              NULL,                       /* no security */
                              OPEN_EXISTING,
                              0,                          /* no threads */
                              NULL);                      /* no templates */
    if(port_handle==INVALID_HANDLE_VALUE) {
        printf("unable to open port %s -> %s\n",COM, portname);
        exit(0);
    }
    strcpy(mode, "baud=57600 data=8 parity=n stop=1");
    memset(&port_sets, 0, sizeof(port_sets));  /* clear the new struct  */
    port_sets.DCBlength = sizeof(port_sets);

    if(!BuildCommDCBA(mode, &port_sets)) {
        printf("dcb settings failed\n");
        CloseHandle(port_handle);
        exit(0);
    }

    if(!SetCommState(port_handle, &port_sets)) {
        printf("cfg settings failed\n");
        CloseHandle(port_handle);
        exit(0);
    }

    timeout_sets.ReadIntervalTimeout         = 1;
    timeout_sets.ReadTotalTimeoutMultiplier  = 1000;
    timeout_sets.ReadTotalTimeoutConstant    = 1;
    timeout_sets.WriteTotalTimeoutMultiplier = 1000;
    timeout_sets.WriteTotalTimeoutConstant   = 1;

    if(!SetCommTimeouts(port_handle, &timeout_sets)) {
        printf("timeout settings failed\n");
        CloseHandle(port_handle);
        exit(0);
    }
}

void putByte(int byte)
{
    int n;
    verbose_print("TX: 0x%02X\n", byte);
    WriteFile(port_handle, &byte, 1, (LPDWORD)((void *)&n), NULL);
    if (n != 1)
        comErr("Serial port failed to send a byte, write returned %d\n", n);
}

void putBytes(unsigned char * data, int len)
{
    /*
    int i;
    for (i = 0; i < len; i++)
        putByte(data[i]);
    */
    int n;
    WriteFile(port_handle, data, len, (LPDWORD)((void *)&n), NULL);
    if (n != len)
        comErr("Serial port failed to send a byte, write returned %d\n", n);
}

int getByte()
{
    unsigned char buf[2];
    int n;
    ReadFile(port_handle, buf, 1, (LPDWORD)((void *)&n), NULL);
    verbose_print(n<1?"RX: fail\n":"RX:  0x%02X\n", buf[0] & 0xFF);
    if (n == 1)
        return buf[0] & 0xFF;
    comErr("Serial port failed to receive a byte, read returned %d\n", n);
    return -1;  // never reached
}
#endif  // not __linux__ nor __APPLE__


/*
 * generic routines
 */
void comErr(char *fmt, ...)
{
    char buf[500];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    fprintf(stderr,"%s", buf);
    perror(COM);
    va_end(va);
    abort();
}

void flsprintf(FILE* f, char *fmt, ...)
{
    char buf[500];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    fprintf(f,"%s", buf);
    fflush(f);
    va_end(va);
}

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

//  get line replacement
size_t getlinex(char **lineptr, size_t *n, FILE *stream)
{
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL)
        return -1;
    if (stream == NULL)
        return -1;
    if (n == NULL)
        return -1;

    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF)
        return -1;
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while (c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }
    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;
    return p - bufptr - 1;
}

void sleep_ms(int num)
{
    struct timespec tspec;
    tspec.tv_sec = num / 1000;
    tspec.tv_nsec = (num % 1000) * 1000000;
    nanosleep(&tspec, 0);
}

void sleep_us(int num)
{
    struct timespec tspec;
    tspec.tv_sec = num / 1000000;
    tspec.tv_nsec = (num % 1000000) * 1000;
    nanosleep(&tspec, 0);
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
    printf("-h : show this help message and exit\n");
}

void parseArgs(int argc, char *argv[])
{
    int c;
    while ((c = getopt(argc, argv, "c:nphs:r:t:v:")) != -1) {
        switch (c) {
        case 'c' :
            COM=optarg;
            break;
        case 'n':
            verify = 0;
            break;
        case 'p':
            program = 0;
            // skip program means also skip verify.
            verify = 0;
            break;
        case 's' :
            sscanf(optarg,"%d",&sleep_time);
            break;
        case 'r' :
            reset = 1;
            sscanf(optarg,"%d",&reset_time);
            break;
        case 't' :
            setCPUtype(optarg);
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
            abort();
        }
    }
    if (argc<=1)
        printHelp();
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
    while ((read = getlinex(&line, &len, sf)) != -1) {
        dump_print("\nRead %d chars: %s",read,line);
        if (line[0] != '#') {
            sscanf(line,"%s %d %d %x %x %s", (char*)&read_cpu_type,
                    &read_flash_size,&read_page_size,&read_id,&read_mask,(char*)&read_algo_type);
            dump_print("\n*** %s,%d,%d,%x,%x,%s", read_cpu_type,
                       read_flash_size, read_page_size, read_id, read_mask, read_algo_type);
            if (strcmp(read_cpu_type,cpu) == 0) {
                flash_size = read_flash_size;
                page_size = read_page_size;
                devid_expected = read_id;
                devid_mask = read_mask;
                info_print("Found database match %s,%d,%d,%x,%x,%s\n", read_cpu_type,
                           read_flash_size, read_page_size, read_id, read_mask, read_algo_type);
                cf = NULL;
                for (i = 0; chip_families[i] != NULL; i++) {
                    if (strcmp(chip_families[i]->name, read_algo_type) == 0) {
                        cf = chip_families[i];
                        break;
                    }
                }
                if (cf != NULL) {
                        chip_family = cf->id;
                        config_size = cf->config_size;
                } else {
                if (strcmp("CF_P16F_A",   read_algo_type) == 0) chip_family = CF_P16F_A;
                if (strcmp("CF_P16F_B",   read_algo_type) == 0) chip_family = CF_P16F_B;
                if (strcmp("CF_P16F_C",   read_algo_type) == 0) chip_family = CF_P16F_C;
                if (strcmp("CF_P16F_D",   read_algo_type) == 0) chip_family = CF_P16F_D;
                if (strcmp("CF_P18F_A",   read_algo_type) == 0) chip_family = CF_P18F_A;
                if (strcmp("CF_P18F_B",   read_algo_type) == 0) chip_family = CF_P18F_B;
                if (strcmp("CF_P18F_C",   read_algo_type) == 0) chip_family = CF_P18F_C;
                if (strcmp("CF_P18F_D",   read_algo_type) == 0) chip_family = CF_P18F_D;
                if (strcmp("CF_P18F_E",   read_algo_type) == 0) chip_family = CF_P18F_E;
                if (strcmp("CF_P18F_F",   read_algo_type) == 0) chip_family = CF_P18F_F;
                if (strcmp("CF_P18F_G",   read_algo_type) == 0) chip_family = CF_P18F_G;
                if (strcmp("CF_P18F_Q",   read_algo_type) == 0) chip_family = CF_P18F_Q;

                if (chip_family == CF_P18F_A)
                    config_size = 16;
                if (chip_family == CF_P18F_B)
                    config_size = 8;
                if (chip_family == CF_P18F_C) {
                    config_size = 16;
                    chip_family = CF_P18F_B;
                }
                if (chip_family == CF_P18F_D)
                    config_size = 16;
                if (chip_family == CF_P18F_E)
                    config_size = 16;
                if (chip_family == CF_P18F_F)
                    config_size = 12;
                if (chip_family == CF_P18F_Q)
                    config_size = 12;
                if (chip_family == CF_P18F_G) {
                    config_size = 10;
                    chip_family = CF_P18F_F;
                }
                }
                debug_print("chip family:%d, config size:%d\n", chip_family, config_size);
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

int p16a_rst_pointer(void)
{
    debug_print("Resetting PC\n");
    if (chip_family == CF_P16F_D)
        putByte(0x09);  // operation number
    else
        putByte(0x03);  // operation number
    putByte(0x00);  // number of bytes remaining
    getByte();  // return result - no check for its value
    return 0;
    }

int p16a_mass_erase(void)
{
    debug_print( "Mass erase\n");
    putByte(0x07);
    putByte(0x00);
    getByte();
    return 0;
}

int p16a_load_config(void)
{
    debug_print("Load config\n");
    putByte(0x04);
    putByte(0x00);
    getByte();
    return 0;
}

int p16a_inc_pointer(unsigned char num)
{
    debug_print( "Inc pointer %d\n", num);
    putByte(0x05);
    putByte(0x01);
    putByte(num);
    getByte();
    return 0;
}

int p16a_program_page(unsigned int ptr, unsigned char num, unsigned char slow)
{
    //	unsigned char i;
    debug_print("Programming page of %d bytes at 0x%4.4x\n", num, ptr);
    putByte(0x08);
    putByte(num+2);
    putByte(num);
    putByte(slow);
    /*
    for (i = 0; i < num; i++)
        putByte(file_image[ptr+i]);
    */
    putBytes(&file_image[ptr], num);
    getByte();
    return 0;
}

int p16a_read_page(unsigned char * data, unsigned char num)
{
    unsigned char i;
    debug_print( "Reading page of %d bytes\n", num);
    putByte(0x06);
    putByte(0x01);
    putByte(num / 2);
    getByte();
    for (i = 0; i < num; i++)
        *data++ = getByte();
    return 0;
}

int p16a_get_devid(void)
{
    unsigned char tdat[20], devid_lo, devid_hi;
    unsigned int retval;
    p16a_rst_pointer();
    p16a_load_config();
    p16a_inc_pointer(6);
    p16a_read_page(tdat, 4);
    devid_hi = tdat[(2*0)+1];
    devid_lo = tdat[(2*0)+0];
    debug_print( "Getting devid - lo:%2.2x,hi:%2.2x\n", devid_lo, devid_hi);
    retval = (((unsigned int)(devid_lo))<<0) + (((unsigned int)(devid_hi))<<8);
    retval = retval & devid_mask;
    return retval;
}

int p16a_get_config(unsigned char n)
{
    unsigned char tdat[20], devid_lo, devid_hi;
    unsigned int retval;
    p16a_rst_pointer();
    p16a_load_config();
    p16a_inc_pointer(n);
    p16a_read_page(tdat, 4);
    devid_hi = tdat[(2*0)+1];
    devid_lo = tdat[(2*0)+0];
    retval = (((unsigned int)(devid_lo))<<0) + (((unsigned int)(devid_hi))<<8);
    debug_print( "Getting config +%d - lo:%2.2x,hi:%2.2x = %4.4x\n", n, devid_lo,
                  devid_hi, retval);
    return retval;
}

int p16a_program_config(void)
{
    p16a_rst_pointer();
    p16a_load_config();
    p16a_inc_pointer(7);
    p16a_program_page(2*0x8007,2,1);
    p16a_program_page(2*0x8008,2,1);
    if ((chip_family==CF_P16F_B)|(chip_family==CF_P16F_D))
        p16a_program_page(2*0x8009,2,1);
    if (chip_family==CF_P16F_D)
        p16a_program_page(2*0x800A,2,1);
    return 0;
}

int p18a_read_page(unsigned char * data, int address, unsigned char num)
{
    unsigned char i;
    debug_print( "Reading page of %d bytes at 0x%6.6x\n", num, address);
    putByte(0x11);
    putByte(0x04);
    putByte(num/2);
    putByte((address>>16)&0xFF);
    putByte((address>>8)&0xFF);
    putByte((address>>0)&0xFF);
    getByte();
    for (i = 0; i < num; i++) {
        *data++ = getByte();
    }
    return 0;
}

int p18a_mass_erase(void)
{
    debug_print( "Mass erase\n");
    putByte(0x13);
    putByte(0x00);
    getByte();
    return 0;
}

int p18b_mass_erase(void)
{
    debug_print( "Mass erase\n");
    putByte(0x23);
    putByte(0x00);
    getByte();
    return 0;
}

int p18d_mass_erase_part(unsigned long data)
{
    debug_print( "Mass erase part of 0x%6.6x\n", data);
    putByte(0x30);
    putByte(0x03);
    putByte((data>>16)&0xFF);
    putByte((data>>8)&0xFF);
    putByte((data>>0)&0xFF);
    getByte();
    return 0;
}

int p18d_mass_erase(void)
{
    debug_print("Mass erase\n");
    p18d_mass_erase_part(0x800104);
    p18d_mass_erase_part(0x800204);
    p18d_mass_erase_part(0x800404);
    p18d_mass_erase_part(0x800804);
    /*
    p18d_mass_erase_part(0x801004);
    p18d_mass_erase_part(0x802004);
    p18d_mass_erase_part(0x804004);
    p18d_mass_erase_part(0x808004);
    */
    p18d_mass_erase_part(0x800004);
    p18d_mass_erase_part(0x800005);
    p18d_mass_erase_part(0x800002);
    return 0;
}

int p18e_mass_erase(void)
{
    debug_print("Mass erase\n");
    p18d_mass_erase_part(0x800104);
    p18d_mass_erase_part(0x800204);
    p18d_mass_erase_part(0x800404);
    p18d_mass_erase_part(0x800804);
    p18d_mass_erase_part(0x801004);
    p18d_mass_erase_part(0x802004);
    p18d_mass_erase_part(0x804004);
    p18d_mass_erase_part(0x808004);
    p18d_mass_erase_part(0x800004);
    p18d_mass_erase_part(0x800005);
    p18d_mass_erase_part(0x800002);
    return 0;
}

int p18a_write_page(unsigned char * data, int address, unsigned char num)
{
    unsigned char i,empty;
    empty = 0;
    for (i = 0; i < num; i++) {
        if (data[i]!=0xFF) {
            empty = 0;
        }
    }
    if (empty) {
        verbose_print("~");
        return 0;
    }
    debug_print("Writing A page of %d bytes at 0x%6.6x\n", num, address);
    putByte(0x12);
    putByte(4+num);
    putByte(num);
    putByte((address>>16)&0xFF);
    putByte((address>>8)&0xFF);
    putByte((address>>0)&0xFF);
    for (i = 0; i < num; i++) {
        putByte(data[i]);

        // Without this delay Arduino Leonardo's USB serial might be stalled
        sleep_us(5);
    }
    getByte();
    return 0;
}

int p18d_write_page(unsigned char * data, int address, unsigned char num)
{
    unsigned char i,empty;
    empty = 0;
    for (i = 0; i < num; i++) {
        if (data[i]!=0xFF)
            empty = 0;
    }
    if (empty) {
        verbose_print("~");
        return 0;
    }
    debug_print("Writing D page of %d bytes at 0x%6.6x\n", num, address);
    putByte(0x31);
    putByte(4+num);
    putByte(num);
    putByte((address>>16)&0xFF);
    putByte((address>>8)&0xFF);
    putByte((address>>0)&0xFF);
    for (i = 0; i < num; i++) {
        putByte(data[i]);

        // Without this delay Arduino Leonardo's USB serial might be stalled
        sleep_us(5);
    }
    getByte();
    return 0;
}

int p18a_write_cfg(unsigned char data1, unsigned char data2, int address)
{
    debug_print( "Writing cfg 0x%2.2x 0x%2.2x at 0x%6.6x\n", data1, data2, address);
    putByte(0x14);
    putByte(6);
    putByte(0);
    putByte((address>>16)&0xFF);
    putByte((address>>8)&0xFF);
    putByte((address>>0)&0xFF);
    putByte(data1);
    putByte(data2);
    getByte();
    return 0;
}

int p18d_write_cfg(unsigned char data1, unsigned char data2, int address)
{
    debug_print("Writing cfg 0x%2.2x 0x%2.2x at 0x%6.6x\n", data1, data2, address);
    putByte(0x32);
    putByte(6);
    putByte(0);
    putByte((address>>16)&0xFF);
    putByte((address>>8)&0xFF);
    putByte((address>>0)&0xFF);
    putByte(data1);
    putByte(data2);
    getByte();
    return 0;
}


int p16c_mass_erase(void)
{
    debug_print("Mass erase\n");
    putByte(0x43);
    putByte(0x00);
    getByte();
    return 0;
}

void p16c_set_pc(unsigned long pc)
{
    pp_ops_write_isp_8(0x80);
    pp_ops_delay_us(2);
    pp_ops_write_isp_24(pc);
}

int p16c_read_page(unsigned char * data, int address, unsigned char num)
{
    uint8_t *p, buf[1024];
    int i, n;

    address /= 2;
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

int p16c_write_page(unsigned char * data, int address, unsigned char num)
{
    unsigned char i, empty;
    address = address / 2;
    empty = 1;
    for (i = 0; i < num; i = i + 2) {
        if	((data[i]!=0xFF)|(data[i+1]!=0xFF))
            empty = 0;
    }
    debug_print("Writing A page of %d bytes at 0x%6.6x\n", num, address);
    if (empty) {
        verbose_print("~");
        return 0;
    }
    putByte(0x42);
    putByte(4+num);
    putByte(num);
    putByte((address>>16)&0xFF);
    putByte((address>>8)&0xFF);
    putByte((address>>0)&0xFF);
    for (i = 0; i < num; i++) {
        putByte(data[i]);

        // Without this delay Arduino Leonardo's USB serial might be stalled
        sleep_us(5);
    }
    getByte();
    return 0;
}

int p16c_get_devid(void)
{
    unsigned char tdat[20],devid_lo,devid_hi;
    unsigned int retval;
    p16c_read_page(tdat, 0x8006*2,4);
    devid_hi = tdat[(2*0)+1];
    devid_lo = tdat[(2*0)+0];
    debug_print("Getting devid - lo:%2.2x,hi:%2.2x\n",devid_lo,devid_hi);
    retval = (((unsigned int)(devid_lo))<<0) + (((unsigned int)(devid_hi))<<8);
    retval = retval & devid_mask;
    return retval;
}

int p16c_write_single_cfg(unsigned char data1, unsigned char data2, int address)
{
    debug_print( "Writing cfg 0x%2.2x 0x%2.2x at 0x%6.6x\n", data1, data2, address);
    putByte(0x44);
    putByte(6);
    putByte(0);
    putByte((address>>16)&0xFF);
    putByte((address>>8)&0xFF);
    putByte((address>>0)&0xFF);
    putByte(data1);
    putByte(data2);
    getByte();
    return 0;
}

int p18q_write_single_cfg(unsigned char data1, unsigned char data2, int address)
{
    debug_print( "Writing cfg 0x%2.2x 0x%2.2x at 0x%6.6x\n", data1, data2, address);
    putByte(0x45);
    putByte(6);
    putByte(0);
    putByte((address>>16)&0xFF);
    putByte((address>>8)&0xFF);
    putByte((address>>0)&0xFF);
    putByte(data1);
    putByte(data2);
    getByte();
    return 0;
}

int p18q_write_byte_cfg(unsigned char data, int address)
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

int p18q_write_page(unsigned char *data, int address, unsigned char num)
{
    uint8_t *p, buf[1024];
    int i, n;

    address /= 2;
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

int p16c_write_cfg(void)
{
    p16c_write_single_cfg(config_bytes[1],config_bytes[0],0x8007);
    p16c_write_single_cfg(config_bytes[3],config_bytes[2],0x8008);
    p16c_write_single_cfg(config_bytes[5],config_bytes[4],0x8009);
    p16c_write_single_cfg(config_bytes[7],config_bytes[6],0x800A);
    p16c_write_single_cfg(config_bytes[9],config_bytes[8],0x800B);
    return 0;
}

int p18q_read_cfg(unsigned char * data, int address, unsigned char num)
{
    uint8_t *p, buf[1024];
    int i, n;

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

unsigned char p16c_enter_progmode(void)
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

int prog_enter_progmode(void)
{
    if (cf)
        return cf->enter_progmode();

    debug_print("Entering programming mode\n");

    if (chip_family==CF_P16F_C) p16c_enter_progmode();
    else if (chip_family==CF_P18F_F) p16c_enter_progmode();
    else if (chip_family==CF_P18F_Q) p16c_enter_progmode();
    else
        printf("Error %s is not implemented\n", __func__);
    return 0;
}

int prog_exit_progmode(void)
{
    if (cf)
        return cf->exit_progmode();

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

int prog_reset(void)
{
    return prog_exit_progmode();
}

int prog_get_device_id(void)
{
    unsigned char mem_str[10];
    unsigned int devid;

    debug_print("getting ID for family %d\n",chip_family);
    if (cf)
        return cf->get_device_id();

    if ((chip_family==CF_P16F_A)|(chip_family==CF_P16F_B)|(chip_family==CF_P16F_D))
        return p16a_get_devid();
    if (chip_family==CF_P16F_C)
        return p16c_get_devid();
    if ((chip_family==CF_P18F_A)|(chip_family==CF_P18F_B)|(chip_family==CF_P18F_D)|(chip_family==CF_P18F_E)) {
        p18a_read_page((unsigned char *)&mem_str, 0x3FFFFE, 2);
        devid = (((unsigned int)(mem_str[1]))<<8) + (((unsigned int)(mem_str[0]))<<0);
        devid = devid & devid_mask;
        return devid;
    }
    if ((chip_family==CF_P18F_F)|(chip_family==CF_P18F_Q)) {
        p16c_read_page(mem_str, 0x3FFFFE*2,2);
        devid = (((unsigned int)(mem_str[1]))<<8) + (((unsigned int)(mem_str[0]))<<0);
        devid = devid & devid_mask;
        return devid;
    }

    return 0;
}

/*
 * hex parse and main function
 */

int parse_hex(char * filename, unsigned char * progmem, unsigned char * config)
{
    char * line = NULL;
    unsigned char line_content[128];
    size_t len = 0;
    int i,temp, read,line_len, line_type, line_address, line_address_offset,effective_address;
    int p16_cfg = 0;
    debug_print("Opening filename %s \n", filename);
    FILE* sf = fopen(filename, "r");
    if (sf == 0) {
        fprintf (stderr, "Can't open hex file %s\n", filename);
        return -1;
    }
    line_address_offset = 0;
    if (chip_family==CF_P16F_A) p16_cfg = 1;
    if (chip_family==CF_P16F_B) p16_cfg = 1;
    if (chip_family==CF_P16F_C) p16_cfg = 1;
    if (chip_family==CF_P16F_D) p16_cfg = 1;

    debug_print("File open\n");
    while ((read = getlinex(&line, &len, sf)) != -1) {
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
            if (cf && cf->config_address == effective_address) {
                dump_print("CB ");
                for (i = 0; i < line_len; i++)
                    config[i] = line_content[i];
            }
            if ((line_address_offset == 0x30) &&
                ((chip_family==CF_P18F_A)|(chip_family==CF_P18F_D)|(chip_family==CF_P18F_E)|
                 (chip_family==CF_P18F_F)|(chip_family==CF_P18F_Q))) {
                dump_print("CB ");
                for (i = 0; i < line_len; i++)
                    config[i] = line_content[i];
            }
            if ((chip_family == CF_P18F_B) && (effective_address == (flash_size-config_size))) {
                dump_print("CB ");
                for (i = 0; i < line_len; i++)
                    config[i] = line_content[i];
            }
            if ((line_address_offset == 0x01) && (p16_cfg == 1)) {
                dump_print("CB ");
                for (i = 0; i < line_len; i++) {
                    if (0 <= line_address + i - 0x0E)
                        config[line_address+i-0x0E] = line_content[i];
                }
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

int main(int argc, char *argv[])
{
    int i, j, pages_performed, config, econfig;
    unsigned char *pm_point, *cm_point;
    unsigned char tdat[200];
    parseArgs(argc, argv);
    // check setCPUtype works or not.
    if (flash_size == 0) {
          printf("Please use '-t MODEL' to specify correct PIC model, such as '16f1824'\n");
          exit(1);
    }
    if (strcmp(COM, "") == 0) {
        printf("Please use '-c PORT' to specify correct serial device\n");
        exit(1);
    }
    info_print("PP programmer, version %s\n", PP_VERSION);

    info_print("Opening serial port\n");
    initSerialPort();

    if (sleep_time > 0) {
        info_print("Sleeping for %d ms while arduino bootloader expires\n", sleep_time);
        fflush(stdout);
        sleep_ms(sleep_time);
    }

    for (i = 0; i < PROGMEM_LEN; i++)
        progmem[i] = 0xFF;  // assume erased memories (0xFF)
    for (i = 0; i < CONFIG_LEN; i++)
        config_bytes[i] = 0xFF;

    char* filename = argv[argc-1];
    pm_point = (unsigned char *)(&progmem);
    cm_point = (unsigned char *)(&config_bytes);
    if (program == 1 && parse_hex(filename,pm_point,cm_point)) {
        // parse and write content of hex file into buffers
        fprintf(stderr,"Failed to read input file.\n");
        abort();
    }

    // now this is ugly kludge
    // my original programmer expected only file_image holding the image of memory to be programmed
    // for PIC18, it is divided into two regions, program memory and config. to glue those two
    // different approaches, I made this. not particulary proud of having this mess
    for (i = 0; i < 70000; i++)
        file_image[i] = progmem[i];
    for (i = 0; i < 10; i++)
        file_image[2*0x8007 + i] = config_bytes[i];
    // for (i = 0; i < 10; i++)
    //     printf("%2.2x", config_bytes[i]);
    for (i = 0; i < 70000; i++) {
        if (i % 2)
            file_image[i] = 0x3F & file_image[i];
    }

    if (reset) {
        printf("Reset the target and wait for %d ms\n", reset_time);
        prog_reset();
        sleep_ms(reset_time);
    }

    prog_enter_progmode();  // enter programming mode and probe the target
    i = prog_get_device_id();
    if (i == devid_expected) {
        info_print("Device ID: %4.4x \n", i);
    } else {
        printf("Wrong device ID: %4.4x, expected: %4.4x\n", i, devid_expected);
        printf("Check for connection to target MCU, exiting now\n");
        prog_exit_progmode();
        return 1;
    }

    if (cf) {
        if (program) {
            pages_performed = 0;
            cf->mass_erase();
            info_print("Programming FLASH (%d B in %d pages per %d bytes): \n", flash_size,
                       flash_size/page_size,page_size);
            fflush(stdout);
            for (i = 0; i < flash_size; i=i+page_size) {
                if (!is_empty(progmem+i, page_size)) {
                    cf->write_page(progmem + i, i, page_size);
                    pages_performed++;
                    info_print("#");
                } else {
                    debug_print(".");
                }
            }

            info_print("\n%d pages programmed\n",pages_performed);
            info_print("Programming config\n");
            cf->write_config(config_bytes, config_size);
        }
        if (verify) {
            pages_performed = 0;
            info_print("Verifying FLASH (%d B in %d pages per %d bytes): \n",flash_size,
                       flash_size/page_size,page_size);
            for (i = 0; i < flash_size; i = i + page_size) {
                if (is_empty(progmem+i,page_size)) {
                    debug_print("#");
                } else {
                    cf->read_page(tdat, i, page_size);
                    pages_performed++;
                    verbose_print("Verifying page at 0x%4.4X\n", i);
                    info_print("#");
                    for (j = 0; j < page_size; j++) {
                        if (progmem[i+j] != tdat[j]) {
                            printf("Error at 0x%4.4X E:0x%2.2X R:0x%2.2X\n", i + j,
                                    progmem[i + j], tdat[j]);
                            printf("Exiting now\n");
                            prog_exit_progmode();
                            exit(0);
                        }
                    }
                }
            }
            info_print("\n%d pages verified\n", pages_performed);

            info_print("Verifying config...");
            cf->read_config(tdat, config_size);
            for (i = 0; i < config_size; i++) {
                if (config_bytes[i] != tdat[i]) {
                    printf("Error at 0x%2.2X E:0x%2.2X R:0x%2.2X\n",i,config_bytes[i],tdat[i]);
                    printf("Exiting now\n");
                    prog_exit_progmode();
                    exit(0);
                }
            }
            info_print("OK\n");
        }
    } else
    // ah, I need to unify programming interfaces for PIC16 and PIC18
    if ((chip_family==CF_P18F_A)|(chip_family==CF_P18F_B)|(chip_family==CF_P18F_D)|
        (chip_family==CF_P18F_E)|(chip_family==CF_P18F_F)|(chip_family==CF_P18F_Q)) {
        //
        // PIC 18F
        //
        if (program) {
            pages_performed = 0;
            if (chip_family==CF_P18F_A)						//erase whole device
                p18a_mass_erase();
            if (chip_family==CF_P18F_B)
                p18b_mass_erase();
            if (chip_family==CF_P18F_D)
                p18d_mass_erase();
            if (chip_family==CF_P18F_E)
                p18e_mass_erase();
            if ((chip_family==CF_P18F_F)|(chip_family==CF_P18F_Q))
                p16c_mass_erase();
            info_print("Programming FLASH (%d B in %d pages per %d bytes): \n", flash_size,
                       flash_size/page_size,page_size);
            fflush(stdout);
            for (i = 0; i < flash_size; i=i+page_size) {
                if (!is_empty(progmem+i, page_size)) {
                    if ((chip_family==CF_P18F_D)|(chip_family==CF_P18F_E))
                        p18d_write_page(progmem + i, i, page_size);
                    else
                    if (chip_family==CF_P18F_F)
                        p16c_write_page(progmem + i, i * 2, page_size);
                    else
                    if (chip_family==CF_P18F_Q)
                        p18q_write_page(progmem + i, i * 2, page_size);
                    else
                        p18a_write_page(progmem + i, i, page_size);
                    pages_performed++;
                    info_print("#");
                } else {
                    debug_print(".");
                }
            }

            info_print("\n%d pages programmed\n",pages_performed);
            info_print("Programming config\n");
            for (i = 0; i < config_size; i = i + 2) {
                // write config bytes for PIC18Fxxxx and 18FxxKxx devices
                if (chip_family==CF_P18F_A)
                    p18a_write_cfg(config_bytes[i], config_bytes[i+1], 0x300000+i);
                if (chip_family==CF_P18F_D)
                    p18d_write_cfg(config_bytes[i], config_bytes[i+1], 0x300000+i);
                if (chip_family==CF_P18F_E)
                    p18d_write_cfg(config_bytes[i], config_bytes[i+1], 0x300000+i);
                if (chip_family==CF_P18F_F)
                    p16c_write_single_cfg(config_bytes[i+1], config_bytes[i], 0x300000+i);
                if (chip_family==CF_P18F_Q)
                    p18q_write_single_cfg(config_bytes[i+1], config_bytes[i], 0x300000+i);
            }
            // for PIC18FxxJxx, config bytes are at the end of FLASH memory
        }
        if (verify) {
            pages_performed = 0;
            info_print("Verifying FLASH (%d B in %d pages per %d bytes): \n",flash_size,
                       flash_size/page_size,page_size);
            for (i = 0; i < flash_size; i = i + page_size) {
                if (is_empty(progmem+i,page_size)) {
                    debug_print("#");
                } else {
                    if ((chip_family==CF_P18F_F)|(chip_family==CF_P18F_Q))
                        p16c_read_page(tdat, i*2, page_size);
                    else
                        p18a_read_page(tdat, i, page_size);
                    pages_performed++;
                    verbose_print("Verifying page at 0x%4.4X\n", i);
                    info_print("#");
                    for (j = 0; j < page_size; j++) {
                        if (progmem[i+j] != tdat[j]) {
                            printf("Error at 0x%4.4X E:0x%2.2X R:0x%2.2X\n", i + j,
                                    progmem[i + j], tdat[j]);
                            printf("Exiting now\n");
                            prog_exit_progmode();
                            exit(0);
                        }
                    }
                }
            }
            info_print("\n%d pages verified\n", pages_performed);
            if ((chip_family==CF_P18F_F)|(chip_family==CF_P18F_Q))
                p16c_read_page(tdat,0x300000*2,page_size);
            else
                p18a_read_page(tdat,0x300000,page_size);

            info_print("Verifying config...");
            for (i = 0; i < config_size; i++) {
                if (config_bytes[i] != tdat[i]) {
                    printf("Error at 0x%2.2X E:0x%2.2X R:0x%2.2X\n",i,config_bytes[i],tdat[i]);
                    printf("Exiting now\n");
                    prog_exit_progmode();
                    exit(0);
                }
            }
            info_print("OK\n");
        }
    } else {
        //
        // PIC 16F
        //
        if (program) {
            if ((chip_family==CF_P16F_A)|(chip_family==CF_P16F_B)|(chip_family==CF_P16F_D))
                p16a_mass_erase();
            if (chip_family==CF_P16F_C)
                p16c_mass_erase();
            if ((chip_family==CF_P16F_A)|(chip_family==CF_P16F_B)|(chip_family==CF_P16F_D))
                p16a_rst_pointer();  // pointer reset is needed before every "big" operation
            info_print("Programming FLASH (%d B in %d pages)", flash_size, flash_size/page_size);
            fflush(stdout);
            for (i = 0; i < flash_size; i = i + page_size) {
                info_print(".");
                if ((chip_family==CF_P16F_A)|(chip_family==CF_P16F_B)|(chip_family==CF_P16F_D))
                    p16a_program_page(i,page_size,0);
                if (chip_family==CF_P16F_C)
                    p16c_write_page(progmem+i,i,page_size);
            }
            info_print("\n");
            info_print("Programming config\n");
            if ((chip_family==CF_P16F_A)|(chip_family==CF_P16F_B)|(chip_family==CF_P16F_D))
                p16a_program_config();
            if (chip_family==CF_P16F_C)
                p16c_write_cfg();
        }
        if (verify) {
            info_print("Verifying FLASH (%d B in %d pages)",flash_size,flash_size/page_size);
            if ((chip_family==CF_P16F_A)|(chip_family==CF_P16F_B)|(chip_family==CF_P16F_D))
                p16a_rst_pointer();
            for (i = 0; i < flash_size; i = i + page_size) {
                info_print(".");
                if ((chip_family==CF_P16F_A)|(chip_family==CF_P16F_B)|(chip_family==CF_P16F_D))
                    p16a_read_page(tdat, page_size);
                if (chip_family==CF_P16F_C)
                    p16c_read_page(tdat, i, page_size);
                for (j = 0; j < page_size; j++) {
                    if (file_image[i+j] != tdat[j]) {
                        printf("Error at 0x%4.4X E:0x%2.2X R:0x%2.2X\n",
                               i + j, file_image[i + j], tdat[j]);
                        prog_exit_progmode();
                        exit(0);
                    }
                }
            }
            info_print("\n");
            info_print("Verifying config\n");
            if ((chip_family==CF_P16F_A)|(chip_family==CF_P16F_B)|(chip_family==CF_P16F_D)) {
                config = p16a_get_config(7);
                econfig = (((unsigned int)(file_image[2*0x8007]))<<0) +
                    (((unsigned int)(file_image[2*0x8007+1]))<<8);
                if (config == econfig) {
                    info_print("config 1 OK: %4.4X\n",config);
                } else {
                    printf("config 1 error: E:0x%4.4X R:0x%4.4X\n", config, econfig);
                }

                config = p16a_get_config(8);
                econfig = (((unsigned int)(file_image[2*0x8008]))<<0) +
                    (((unsigned int)(file_image[2*0x8008+1]))<<8);
                if (config == econfig) {
                    info_print("config 2 OK: %4.4X\n", config);
                } else {
                    printf("config 2 error: E:0x%4.4X R:0x%4.4X\n",config,econfig);
                }
            }
            if (chip_family==CF_P16F_C) {
                p16c_read_page(tdat,0x8007*2,page_size);
                for (j = 0; j < 10; j++) {
                    if (config_bytes[j] != tdat[j]) {
                        printf("Error at 0x%4.4X E:0x%2.2X R:0x%2.2X\n",
                                i + j, config_bytes[j], tdat[j]);
                        prog_exit_progmode();
                        exit(0);
                    }
                }
            }
        }
    }
    prog_exit_progmode();
    return 0;
}
