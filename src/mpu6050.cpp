#include "mpu6050_reg.hpp"
#include "mpu6050.hpp"
#include "misc.hpp"
#include <stdlib.h>

#define I2C_USCI_BASE USCI_B1_BASE
#define I2C_USCI_VECTOR USCI_B1_VECTOR
#define I2C_USCI_IV UCB1IV

static uint8_t initMPU(uint8_t regAddr,
                       uint8_t bitStart,
                       uint8_t length,
                       uint8_t source);
static void readByte(uint8_t regAddr, uint8_t* b);
static void readBytes(uint8_t regAddr,
                      uint8_t length,
                      uint8_t* data);
void writeBit(uint8_t regAddr, uint8_t bitNum, uint8_t data);

bool MPU6050::DataReady(){
    return P7IN | GPIO_PIN0;
}

void MPU6050::hwInit()
{
    P7DIR &= ~GPIO_PIN0;

    P4SEL |= GPIO_PIN1 | GPIO_PIN2;
    P4REN |= GPIO_PIN1 | GPIO_PIN2;
    P4OUT |= GPIO_PIN1 | GPIO_PIN2;

    USCI_B_I2C_initMasterParam param = {0};
    param.selectClockSource = USCI_B_I2C_CLOCKSOURCE_SMCLK;
    param.i2cClk = UCS_getSMCLK();
    param.dataRate = USCI_B_I2C_SET_DATA_RATE_400KBPS;
    USCI_B_I2C_initMaster(I2C_USCI_BASE, &param);
    USCI_B_I2C_enable(I2C_USCI_BASE);

    // Specify slave address
    USCI_B_I2C_setSlaveAddress(I2C_USCI_BASE, MPU6050Addr);
    USCI_B_I2C_clearInterrupt(I2C_USCI_BASE, USCI_B_I2C_RECEIVE_INTERRUPT);
    USCI_B_I2C_enableInterrupt(I2C_USCI_BASE, USCI_B_I2C_RECEIVE_INTERRUPT);
    USCI_B_I2C_clearInterrupt(I2C_USCI_BASE, USCI_B_I2C_TRANSMIT_INTERRUPT);
    USCI_B_I2C_enableInterrupt(I2C_USCI_BASE, USCI_B_I2C_TRANSMIT_INTERRUPT);
}

void MPU6050::initializeIMU()
{
    // Set MPU6050 clock
    setClockSource(MPU6050_CLOCK_PLL_XGYRO);
    delay_ms(10);
    // Gyroscope sensitivity set to 2000 degrees/sec
    setFullScaleGyroRange(MPU6050_GYRO_FS_2000);
    delay_ms(10);
    // Accelerometer sensitivity set to 4g
    setFullScaleAccelRange(MPU6050_ACCEL_FS_4);
    delay_ms(10);
    // Wake up device.
    setSleepEnabled(0);
    delay_ms(10);
    writeBit(MPU6050_RA_INT_ENABLE, MPU6050_INTERRUPT_DATA_RDY_BIT, 1);
}

uint8_t MPU6050::getDeviceID()
{
    uint8_t b = 0;
    readByte(MPU6050_RA_WHO_AM_I, &b);
    return b;
}

#define I2C_BUF_LENGTH 32
static char i2c_buf[I2C_BUF_LENGTH];
static size_t i2c_buf_len = 0;
static size_t i2c_buf_cur = 0;

static uint8_t *i2c_rx_buf = NULL;
static size_t i2c_rx_buf_len = 0;

__attribute__((interrupt(I2C_USCI_VECTOR)))
void USCI_B0_ISR(void) {
    switch (__even_in_range(I2C_USCI_IV, 12)) {
        case USCI_I2C_UCTXIFG:
            if (i2c_buf_cur < i2c_buf_len) {
                USCI_B_I2C_masterSendMultiByteNext( I2C_USCI_BASE, i2c_buf[i2c_buf_cur]);
                i2c_buf_cur++;
            } else {
                USCI_B_I2C_masterSendMultiByteStop(I2C_USCI_BASE);
                //Clear master interrupt status
                USCI_B_I2C_clearInterrupt(I2C_USCI_BASE,
                                          USCI_B_I2C_TRANSMIT_INTERRUPT);
                __bic_SR_register_on_exit(LPM0_bits);
            }
            break;
        case USCI_I2C_UCRXIFG:
            i2c_rx_buf_len--;
            if(i2c_rx_buf_len){
                if(i2c_rx_buf_len== 1) {
                    //Initiate end of reception -> Receive byte with NAK
                    *i2c_rx_buf++ = USCI_B_I2C_masterReceiveMultiByteFinish( I2C_USCI_BASE);
                } else {
                    //Keep receiving one byte at a time
                    *i2c_rx_buf++ = USCI_B_I2C_masterReceiveMultiByteNext( I2C_USCI_BASE);
                }
            } else {
                //Receive last byte
                *i2c_rx_buf= USCI_B_I2C_masterReceiveMultiByteNext( I2C_USCI_BASE);
                __bic_SR_register_on_exit(LPM0_bits);
            }
            break;
    }
}

/* write a byte to specific register, cannot called in interrupt context */
void writeByte(uint8_t regAddr, uint8_t data)
{
    while (USCI_B_I2C_isBusBusy(I2C_USCI_BASE))
        ;

    USCI_B_I2C_setMode(I2C_USCI_BASE, USCI_B_I2C_TRANSMIT_MODE);

    // Initiate start and send first character
    i2c_buf[0] = regAddr;
    i2c_buf[1] = data;
    i2c_buf_cur = 1;
    i2c_buf_len = 2;
    USCI_B_I2C_masterSendMultiByteStart(I2C_USCI_BASE, i2c_buf[0]);

    // wait for end
    __bis_SR_register(GIE + LPM0_bits);
    __no_operation();
}

/* set/clear some bit to specific register, cannot called in interrupt context */
void writeBit(uint8_t regAddr, uint8_t bitNum, uint8_t data)
{
    uint8_t b = 0;
    readByte(regAddr, &b);
    delay_ms(2);
    b = (data != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
    writeByte(regAddr, b);
}

/* read some byte from specific register, cannot called in interrupt context */
static void readByte(uint8_t regAddr, uint8_t* b)
{
    // send address
    USCI_B_I2C_setMode(I2C_USCI_BASE, USCI_B_I2C_TRANSMIT_MODE);
    i2c_buf_cur = 1;
    i2c_buf_len = 1;
    USCI_B_I2C_masterSendSingleByte(I2C_USCI_BASE, regAddr);

    // receive
    USCI_B_I2C_setMode(I2C_USCI_BASE, USCI_B_I2C_RECEIVE_MODE);
    i2c_rx_buf = b;
    i2c_rx_buf_len = 1;
    USCI_B_I2C_masterReceiveSingleStart(I2C_USCI_BASE);

    // wait for end
    __bis_SR_register(GIE + LPM0_bits);
    __no_operation();
}

static void readBytes(uint8_t regAddr,
                      uint8_t length,
                      uint8_t* data)
{
    // send address
    USCI_B_I2C_setMode(I2C_USCI_BASE, USCI_B_I2C_TRANSMIT_MODE);
    USCI_B_I2C_masterSendSingleByte(I2C_USCI_BASE, regAddr);

    // receive
    USCI_B_I2C_setMode(I2C_USCI_BASE, USCI_B_I2C_RECEIVE_MODE);
    i2c_rx_buf = data;
    i2c_rx_buf_len = length;
    USCI_B_I2C_masterReceiveMultiByteStart(I2C_USCI_BASE);

    // wait for end
    __bis_SR_register(GIE + LPM0_bits);
    __no_operation();
}

uint8_t MPU6050::setFullScaleAccelRange(uint8_t range)
{
    return initMPU(MPU6050_RA_ACCEL_CONFIG,
                   MPU6050_ACONFIG_AFS_SEL_BIT, MPU6050_ACONFIG_AFS_SEL_LENGTH,
                   range);
}

void MPU6050::setFullScaleGyroRange(uint8_t range)
{
    initMPU(MPU6050_RA_GYRO_CONFIG, MPU6050_GCONFIG_FS_SEL_BIT,
            MPU6050_GCONFIG_FS_SEL_LENGTH, range);
}

void MPU6050::setClockSource(uint8_t source)
{
    initMPU(MPU6050_RA_PWR_MGMT_1, MPU6050_PWR1_CLKSEL_BIT,
            MPU6050_PWR1_CLKSEL_LENGTH, source);
}

void MPU6050::setSleepEnabled(uint8_t enabled)
{
    writeBit(MPU6050_RA_PWR_MGMT_1, MPU6050_PWR1_SLEEP_BIT,
             enabled);
}

static uint8_t initMPU(uint8_t regAddr,
                       uint8_t bitStart,
                       uint8_t length,
                       uint8_t source)
{
    uint8_t b = 0;

    readByte(regAddr, &b);
    delay_ms(2);
    uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    source <<= (bitStart - length + 1);  // shift data into correct position
    source &= mask;                      // zero all non-important bits in data

    b &= ~(mask);  // zero all important bits in existing byte
    b |= source;   // combine data with existing byte

    writeByte(regAddr, b);
    return b;
}

static uint8_t buffer[14];
void MPU6050::getMotion6(int16_t* ax,
                int16_t* ay,
                int16_t* az,
                int16_t* gx,
                int16_t* gy,
                int16_t* gz)
{
    readBytes(MPU6050_RA_ACCEL_XOUT_H, 14, buffer);
    *ax = (((int16_t)buffer[0]) << 8) | buffer[1];
    *ay = (((int16_t)buffer[2]) << 8) | buffer[3];
    *az = (((int16_t)buffer[4]) << 8) | buffer[5];
    *gx = (((int16_t)buffer[8]) << 8) | buffer[9];
    *gy = (((int16_t)buffer[10]) << 8) | buffer[11];
    *gz = (((int16_t)buffer[12]) << 8) | buffer[13];
}
