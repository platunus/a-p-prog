#include "pp3.h"
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
//#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#if defined(__linux__) || defined(__APPLE__)
#include <termios.h>
#else
#include <windows.h>
#endif

static void comErr(char *fmt, ...);

int baudRate, com;

/*
 * serial IO interfaces for Linux and windows
 */
#if defined(__linux__) || defined(__APPLE__)

void initSerialPort()
{
    baudRate = B57600;
    debug_print("Opening: %s at %d\n", comm_port_name, baudRate);
    com =  open(comm_port_name, O_RDWR | O_NOCTTY | O_NDELAY);
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
        perror(comm_port_name);
        printf("set attr error");
        exit(1);
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
    strcat(portname, comm_port_name);
    port_handle = CreateFileA(portname,
                              GENERIC_READ|GENERIC_WRITE,
                              0,                          /* no share  */
                              NULL,                       /* no security */
                              OPEN_EXISTING,
                              0,                          /* no threads */
                              NULL);                      /* no templates */
    if(port_handle==INVALID_HANDLE_VALUE) {
        printf("unable to open port %s -> %s\n", comm_port_name, portname);
        exit(1);
    }
    strcpy(mode, "baud=57600 data=8 parity=n stop=1");
    memset(&port_sets, 0, sizeof(port_sets));  /* clear the new struct  */
    port_sets.DCBlength = sizeof(port_sets);

    if(!BuildCommDCBA(mode, &port_sets)) {
        printf("dcb settings failed\n");
        CloseHandle(port_handle);
        exit(1);
    }

    if(!SetCommState(port_handle, &port_sets)) {
        printf("cfg settings failed\n");
        CloseHandle(port_handle);
        exit(1);
    }

    timeout_sets.ReadIntervalTimeout         = 1;
    timeout_sets.ReadTotalTimeoutMultiplier  = 1000;
    timeout_sets.ReadTotalTimeoutConstant    = 1;
    timeout_sets.WriteTotalTimeoutMultiplier = 1000;
    timeout_sets.WriteTotalTimeoutConstant   = 1;

    if(!SetCommTimeouts(port_handle, &timeout_sets)) {
        printf("timeout settings failed\n");
        CloseHandle(port_handle);
        exit(1);
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
static void comErr(char *fmt, ...)
{
    char buf[500];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    fprintf(stderr,"%s", buf);
    perror(comm_port_name);
    va_end(va);
    abort();
}
