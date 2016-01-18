#ifndef MISC_H__
#define MISC_H__

#include "driverlib.h"

void _delay_cycles(register uint16_t delayCycles) {
    __asm__ (
             "    sub   #20, %[delayCycles]\n"
             "1:  sub   #4, %[delayCycles] \n"
             "    nop                      \n"
             "    jc    1b                 \n"
             "    inv   %[delayCycles]     \n"
             "    rla   %[delayCycles]     \n"
             "    add   %[delayCycles], r0 \n"
             "    nop                      \n"
             "    nop                      \n"
             "    nop                      \n"
             :                                 // no output
             : [delayCycles] "r" (delayCycles) // input
             :                                 // no memory clobber
            );
}

static void __inline__ delay_ms(int ms){
    _delay_cycles(ms * UCS_getMCLK() / 1000 );
}

#endif
