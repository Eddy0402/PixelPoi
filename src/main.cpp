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

void transmitPicture(void);

int main(void)
{
    // Stop WDT_A
    WDT_A_hold(WDT_A_BASE);

	transmitPicture();

    //LedController::init();
    //LedController::appSetup();

    __bis_SR_register(LPM0_bits + GIE);
}
