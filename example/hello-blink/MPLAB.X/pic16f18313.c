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
 *       Device            :  PIC16F18313
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

#if defined _16F18313

#include <xc.h>
#include "mcc.h"

// CONFIG1
#pragma config FEXTOSC = OFF    // FEXTOSC External Oscillator mode Selection bits->Oscillator not enabled
#pragma config RSTOSC = HFINT1  // Power-up default value for COSC bits->HFINTOSC (1MHz)
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit->CLKOUT function is disabled; I/O or oscillator function on OSC2
#pragma config CSWEN = ON       // Clock Switch Enable bit->Writing to NOSC and NDIV is allowed
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable->Fail-Safe Clock Monitor is enabled

// CONFIG2
#pragma config MCLRE = ON       // Master Clear Enable bit->MCLR/VPP pin function is MCLR; Weak pull-up enabled
#pragma config PWRTE = OFF      // Power-up Timer Enable bit->PWRT disabled
#pragma config WDTE = OFF       // Watchdog Timer Enable bits->WDT disabled; SWDTEN is ignored
#pragma config LPBOREN = OFF    // Low-power BOR enable bit->ULPBOR disabled
#pragma config BOREN = ON       // Brown-out Reset Enable bits->Brown-out Reset enabled, SBOREN bit ignored
#pragma config BORV = LOW       // Brown-out Reset Voltage selection bit->Brown-out voltage (Vbor) set to 2.45V
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit->The PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable bit->Stack Overflow or Underflow will cause a Reset
#pragma config DEBUG = OFF      // Debugger enable bit->Background debugger disabled

// CONFIG3
#pragma config WRT = OFF        // User NVM self-write protection bits->Write protection off
#pragma config LVP = ON         // Low Voltage Programming Enable bit->Low voltage programming enabled. MCLR/VPP pin function is MCLR. MCLRE configuration bit is ignored.

// CONFIG4
#pragma config CP = OFF         // User NVM Program Memory Code Protection bit->User NVM code protection disabled
#pragma config CPD = OFF        // Data NVM Memory Code Protection bit->Data NVM code protection disabled

void OSCILLATOR_Initialize(void)
{
    // NOSC HFINTOSC; NDIV 4;
    OSCCON1 = 0x62;
    // CSWHOLD may proceed; SOSCPWR Low power; SOSCBE crystal oscillator;
    OSCCON3 = 0x00;
    // LFOEN disabled; ADOEN disabled; SOSCEN disabled; EXTOEN disabled; HFOEN disabled;
    OSCEN = 0x00;
    // HFFRQ 4_MHz;
    OSCFRQ = 0x03;
    // HFTUN 0;
    OSCTUNE = 0x00;
}

void WDT_Initialize(void)
{
    // WDTPS 1:65536; SWDTEN OFF;
    WDTCON = 0x16;
}

void PMD_Initialize(void)
{
    // CLKRMD CLKR enabled; SYSCMD SYSCLK enabled; FVRMD FVR enabled; IOCMD IOC enabled; NVMMD NVM enabled;
    PMD0 = 0x00;
    // TMR0MD TMR0 enabled; TMR1MD TMR1 enabled; TMR2MD TMR2 enabled; NCOMD DDS(NCO) enabled;
    PMD1 = 0x00;
    // DACMD DAC enabled; CMP1MD CMP1 enabled; ADCMD ADC enabled;
    PMD2 = 0x00;
    // CCP2MD CCP2 enabled; CCP1MD CCP1 enabled; PWM6MD PWM6 enabled; PWM5MD PWM5 enabled; CWG1MD CWG1 enabled;
    PMD3 = 0x00;
    // MSSP1MD MSSP1 enabled; UART1MD EUSART enabled;
    PMD4 = 0x00;
    // DSMMD DSM enabled; CLC1MD CLC1 enabled; CLC2MD CLC2 enabled;
    PMD5 = 0x00;
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

}

void PIN_MANAGER_IOC(void)
{
}

void SYSTEM_Initialize(void)
{

    PMD_Initialize();
    PIN_MANAGER_Initialize();
    OSCILLATOR_Initialize();
    WDT_Initialize();
}

#endif	// _16F18313
