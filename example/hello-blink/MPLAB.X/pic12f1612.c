/**
 * Generated Source File
 *
 * This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs
 *
 * Description:
 *   This header file provides implementations for driver APIs for all modules
 *   selected in the GUI.
 *   Generation Information :
 *       Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
 *       Device            :  PIC12F1612
 *       Driver Version    :  2.00
 */

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

#if defined _12F1612

#include <xc.h>
#include "mcc.h"

// CONFIG1
#pragma config FOSC = INTOSC        // Oscillator Selection Bits->INTOSC oscillator: I/O function on CLKIN pin
#pragma config PWRTE = OFF          // Power-up Timer Enable->PWRT disabled
#pragma config MCLRE = ON           // MCLR Pin Function Select->MCLR/VPP pin function is MCLR
#pragma config CP = OFF             // Flash Program Memory Code Protection->Program memory code protection is disabled
#pragma config BOREN = ON           // Brown-out Reset Enable->Brown-out Reset enabled
#pragma config CLKOUTEN = OFF       // Clock Out Enable->CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin

// CONFIG2
#pragma config WRT = OFF            // Flash Memory Self-Write Protection->Write protection off
#pragma config ZCD = OFF            // Zero Cross Detect Disable Bit->ZCD disable.  ZCD can be enabled by setting the ZCDSEN bit of ZCDCON
#pragma config PLLEN = OFF          // PLL Enable Bit->4x PLL is enabled when software sets the SPLLEN bit
#pragma config STVREN = ON          // Stack Overflow/Underflow Reset Enable->Stack Overflow or Underflow will cause a Reset
#pragma config BORV = LO            // Brown-out Reset Voltage Selection->Brown-out Reset Voltage (Vbor), low trip point selected.
#pragma config LPBOR = OFF          // Low-Power Brown Out Reset->Low-Power BOR is disabled
#pragma config LVP = ON             // Low-Voltage Programming Enable->Low-voltage programming enabled

// CONFIG3
#pragma config WDTCPS = WDTCPS1F    // WDT Period Select->Software Control (WDTPS)
#pragma config WDTE = OFF           // Watchdog Timer Enable->WDT disabled
#pragma config WDTCWS = WDTCWSSW    // WDT Window Select->Software WDT window size control (WDTWS bits)
#pragma config WDTCCS = SWC         // WDT Input Clock Selector->Software control, controlled by WDTCS bits

void OSCILLATOR_Initialize(void)
{
    // SCS FOSC; SPLLEN disabled; IRCF 500KHz_MF;
    OSCCON = 0x38;
    // TUN 0;
    OSCTUNE = 0x00;
    // SBOREN disabled; BORFS disabled;
    BORCON = 0x00;
}

void PIN_MANAGER_Initialize(void)
{
    /**
    LATx registers
    */
    LATA = 0x00;

    /**
    TRISx registers
    */
    TRISA = 0x17;

    /**
    ANSELx registers
    */
    ANSELA = 0x17;

    /**
    WPUx registers
    */
    WPUA = 0x00;
    OPTION_REGbits.nWPUEN = 1;

    /**
    ODx registers
    */
    ODCONA = 0x00;

    /**
    SLRCONx registers
    */
    SLRCONA = 0x37;

    /**
    INLVLx registers
    */
    INLVLA = 0x3F;

    /**
    APFCONx registers
    */
    APFCON = 0x00;

}

void PIN_MANAGER_IOC(void)
{
}

void SYSTEM_Initialize(void)
{
    PIN_MANAGER_Initialize();
    OSCILLATOR_Initialize();
}

#endif	// _12F1612
