#ifndef TLC5955_H
#define TLC5955_H

#include <stdint.h>

struct __attribute__((__packed__)) ControlData {
    uint16_t DOTCOR[21];
    uint16_t MC_R : 3;
    uint16_t MC_G : 3;
    uint16_t MC_B : 3;
    uint16_t BC_R : 7;
    uint16_t BC_G : 7;
    uint16_t BC_B : 7;
    uint16_t DSPRPT : 1;
    uint16_t TWGRST : 1;
    uint16_t RFRESH : 1;
    uint16_t ESPWM : 1;
    uint16_t LSDVLT : 1;

    uint16_t *operator()() { return reinterpret_cast<uint16_t *>(this); };
};

class TLC5955
{
public:
    void setLED(uint8_t color, uint8_t channel, uint16_t gs);
    void setAllLED(uint8_t color, uint16_t gs);
    void setDCData(uint8_t color, uint8_t channel, uint8_t dc);

    ControlData *getControlData() { return &this->controlData; }
    uint16_t *getGSData() { return this->gsData; }
    void setGSData(uint16_t *data) { this->gsData = data; }
    enum Color : uint16_t { R, G, B };

private:
    struct ControlData controlData;
    uint16_t *gsData;
};

#endif
