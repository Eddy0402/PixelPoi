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

#include <msp430.h>

#include <stdlib.h>

int test(void);

static void __attribute__((naked, section(".crt_0042"), used))
disable_watchdog(void)
{
    __asm__("MOV.W	#23168, &0x015C");
}

int main(void)
{
    test();
    while(1);
}
