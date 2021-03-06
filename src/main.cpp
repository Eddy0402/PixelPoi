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
#include "mpu6050.hpp"
#include "misc.hpp"

static void __attribute__((naked, section(".crt_0042"), used))
disable_watchdog(void)
{
    __asm__("MOV.W	#23168, &0x015C");
}

#define UCS_MCLK_DESIRED_FREQUENCY_IN_KHZ   12000
#define UCS_MCLK_FLLREF_RATIO   366

/*
 * Init clock system as follow:
 * MCLK: 12 MHz
 * SMCLK: 12 MHz
 * ACLK: 32.767 KHz
 */
void initUCS(){
    //Set VCore = 1 for 12MHz clock
    PMM_setVCore(PMM_CORE_LEVEL_2);

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

static int16_t ax = 32767, ay = 32767, az = 32767;
static int16_t gx = 32767, gy = 32767, gz = 32767;

uint8_t mode = 0;
uint8_t id = 0;
uint16_t time = 0;

int main(void)
{
    initUCS();

    MPU6050::hwInit();
    MPU6050::initializeIMU();

    LedController::init();
    LedController::appSetup();
    LedController::start();

    __bis_SR_register(LPM0_bits + GIE);

    P1DIR = GPIO_PIN0;
    while(1){
//        if(MPU6050::DataReady){
//            MPU6050::getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
//        }
        P1OUT ^= GPIO_PIN0;
        time++;

        getImgRange(
                    mode,
                    id,
                    time,
                    reinterpret_cast<uint8_t *>(LedController::getTLCModule(0)->getGSData()),
                    0,
                    16
                   );
        getImgRange(
                    mode,
                    id,
                    time,
                    reinterpret_cast<uint8_t *>(LedController::getTLCModule(1)->getGSData()),
                    17,
                    32
                   );


        __bis_SR_register(LPM0_bits + GIE);
    }
}
