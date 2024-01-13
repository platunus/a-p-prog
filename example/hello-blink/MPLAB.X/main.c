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
        RA5 = 1;
        __delay_ms(1000);
        RA5 = 0;
        __delay_ms(1000);
    }
}
