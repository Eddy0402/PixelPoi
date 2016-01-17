//***************************************************************************************
//  MSP430 Blink the LED Demo - Software Toggle P1.0
//
//  Description; Toggle P1.0 by xor'ing P1.0 inside of a software loop.
//  ACLK = n/a, MCLK = SMCLK = default DCO
//
//                MSP430x5xx
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//            |             P1.0|-->LED
//
//  J. Stevenson
//  Texas Instruments, Inc
//  July 2011
//  Built with Code Composer Studio v5
//***************************************************************************************

#include <stdlib.h>
#include "driverlib.h"
#include "led.hpp"

#define UCS_MCLK_DESIRED_FREQUENCY_IN_KHZ   12000
#define UCS_MCLK_FLLREF_RATIO   366

/*
 * Init clock system as follow:
 * MCLK:
 * SMCLK:
 * ACLK:
 */
void initUCS(){
    //Set VCore = 1 for 12MHz clock
    PMM_setVCore(PMM_CORE_LEVEL_1);

    //Set DCO FLL reference = REFO
    UCS_initClockSignal(
        UCS_FLLREF,
        UCS_REFOCLK_SELECT,
        UCS_CLOCK_DIVIDER_1
        );
    //Set ACLK = REFO
    UCS_initClockSignal(
        UCS_ACLK,
        UCS_REFOCLK_SELECT,
        UCS_CLOCK_DIVIDER_1
        );

    //Set Ratio and Desired MCLK Frequency  and initialize DCO
    UCS_initFLLSettle(
        UCS_MCLK_DESIRED_FREQUENCY_IN_KHZ,
        UCS_MCLK_FLLREF_RATIO
        );

    // Enable global oscillator fault flag
    SFR_clearInterrupt(SFR_OSCILLATOR_FAULT_INTERRUPT);
    SFR_enableInterrupt(SFR_OSCILLATOR_FAULT_INTERRUPT);

    // Enable global interrupt
    __bis_SR_register(GIE);
}

uint16_t status;
__attribute__((interrupt(UNMI_VECTOR)))
void NMI_ISR(void)
{
    do
    {
        // If it still can't clear the oscillator fault flags after the timeout,
        // trap and wait here.
        status = UCS_clearAllOscFlagsWithTimeout(1000);
    }
    while(status != 0);
}

int main(void)
{
    // Stop WDT_A
    WDT_A_hold(WDT_A_BASE);

    initUCS();

    // Enable global interrupt
    __bic_SR_register(GIE);

    LedController::init();
    LedController::appSetup();
    LedController::start();

    __bis_SR_register(LPM0_bits + GIE);

    while(1); /* should not reach here */
}
