/*
 *  (c) 2018 Microchip Technology Inc. and its subsidiaries.
 *
 *  Subject to your compliance with these terms, you may use Microchip
 *  software and any derivatives exclusively with Microchip products. It is
 *  your responsibility to comply with third party
 *  license terms applicable to your use of third party software (including
 *  open source software) that may accompany Microchip software.
 *
 *  THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 *  EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
 *  IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
 *  FOR A PARTICULAR PURPOSE.
 *
 *  IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 *  INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 *  WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
 *  HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
 *  THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
 *  CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
 *  OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
 *  SOFTWARE.
 */

#ifndef MCC_H
#define MCC_H

#if defined _16F1503
#define _XTAL_FREQ 500000
#define PORT_LED A5

#elif defined _12F1612
#define _XTAL_FREQ 500000
#define PORT_LED A5

#elif defined _16F18313
#define _XTAL_FREQ 1000000
#define PORT_LED A5

#elif defined _16F18857
#define _XTAL_FREQ 1000000
#define PORT_LED A0

#else  // MCU SPECIFIC
#error unsupported MCU

#endif  // MCU SPECIFIC

#define INPUT   1
#define OUTPUT  0

#define HIGH    1
#define LOW     0

#define ANALOG      1
#define DIGITAL     0

#define PULL_UP_ENABLED      1
#define PULL_UP_DISABLED     0

#define IO_CAT(x, y) IO_CAT_(x, y)
#define IO_CAT_(x, y) x ## y

// get/set IO aliases
#define IO_TRIS(P)              IO_CAT(TRISAbits.TRIS, P)
#define IO_LAT(P)               IO_CAT(LATAbits.LAT, P)
#define IO_PORT(P)              IO_CAT(PORTAbits.R, P)
#define IO_WPU(P)               IO_CAT(WPUAbits.WPU, P)
#define IO_OD(P)                IO_CAT(ODCONAbits.OD, P)

/**
 * @Param
    none
 * @Returns
    none
 * @Description
    Initializes the device to the default states configured in the
 *                  MCC GUI
 * @Example
    SYSTEM_Initialize(void);
 */
void SYSTEM_Initialize(void);

/**
 * @Param
    none
 * @Returns
    none
 * @Description
    Interrupt on Change Handling routine
 * @Example
    PIN_MANAGER_IOC();
 */
void PIN_MANAGER_IOC(void);

#endif	// MCC_H
