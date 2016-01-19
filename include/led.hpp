#ifndef LED_H__
#define LED_H__

#include "tlc5955.hpp"

namespace LedController{

    void init();
    void start();
    void stop();
    void appSetup();

    TLC5955 *getTLCModule(int n);

    extern uint16_t globalLight;
    extern TLC5955 chip[];
}

#endif
