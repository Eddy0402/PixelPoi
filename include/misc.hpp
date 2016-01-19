#ifndef MISC_H__
#define MISC_H__

#include "driverlib.h"

static void __inline__ _delay_cycles(register uint16_t delayCycles) {
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

extern "C"{
    extern const unsigned char imagedata[];
}

uint16_t getImg(uint8_t mode, uint8_t id, uint16_t time, uint8_t *buf);
uint16_t getImgRange(uint8_t mode, uint8_t id, uint16_t time, uint8_t *buf, uint8_t start, uint8_t end);

#endif
