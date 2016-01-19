#ifndef MPU6050_H_
#define MPU6050_H_

#include <driverlib.h>

namespace MPU6050{

    void getMotion6(int16_t* ax, int16_t* ay, int16_t* az, int16_t* gx, int16_t* gy, int16_t* gz);
    uint8_t getDeviceID();
    void setClockSource(uint8_t source);
    void setFullScaleGyroRange(uint8_t range);
    uint8_t setFullScaleAccelRange(uint8_t range);
    void setSleepEnabled(uint8_t enabled);
    bool DataReady();

    void hwInit();
    void initializeIMU();
}

#endif
