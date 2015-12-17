#include "tlc5955.hpp"

void TLC5955::setLED(uint8_t color, uint8_t channel, uint16_t gs)
{
    gsData[3 * channel + color] = gs;
}

void TLC5955::setAllLED(uint8_t color, uint16_t gs)
{
    uint8_t channel = 16;
    while (channel) gsData[3 * (--channel) + color] = gs;
}

void TLC5955::setDCData(uint8_t color, uint8_t channel, uint8_t dc)
{
    uint16_t bit = 21 * channel + 7 * color;
    this->controlData.DOTCOR[bit / 8] = dc | (0xAF << (bit % 8));
    this->controlData.DOTCOR[bit / 8 + 1] = dc | (0xAF >> (8 - bit % 8));
}
