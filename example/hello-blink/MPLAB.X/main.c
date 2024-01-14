/*
 * small LED blink program
 */

#include <xc.h>

#include "mcc.h"

void main(void)
{
    // initialize the device
    SYSTEM_Initialize();

    while(1) {
        IO_LAT(PORT_LED) = HIGH;
        __delay_ms(1000);
        IO_LAT(PORT_LED) = LOW;
        __delay_ms(1000);
    }
}
