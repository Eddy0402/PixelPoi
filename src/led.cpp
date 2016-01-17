#include "led.hpp"
#include "driverlib.h"
#include "tlc5955.hpp"

uint16_t LedController::globalLight = 0xFFFF;
TLC5955 LedController::chip0;
static int writeControl;

void LedController::init() {

    // P3.3 (UCA0SIMO)
    P3SEL |= GPIO_PIN3;
    // P2.7 (UCA0CLK)
    P2SEL |= GPIO_PIN7;
    // P1.0 (LAT)
    GPIO_setAsOutputPin( GPIO_PORT_P4, GPIO_PIN1);
    GPIO_setOutputLowOnPin( GPIO_PORT_P4, GPIO_PIN1);
    // P1.0 (LED1)
    GPIO_setAsOutputPin( GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN0);
    // SMCLK as GSCLK
    P2SEL |= GPIO_PIN2;
    P2DIR |= GPIO_PIN2;

    // USCI_A0 (SPI Master)
    USCI_A_SPI_initMasterParam spiMasterParam = {0};
    spiMasterParam.selectClockSource = USCI_A_SPI_CLOCKSOURCE_SMCLK;
    spiMasterParam.clockSourceFrequency = UCS_getSMCLK();
    spiMasterParam.desiredSpiClock = 1000000;
    spiMasterParam.msbFirst = USCI_A_SPI_MSB_FIRST;
    spiMasterParam.clockPhase = USCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;//
    spiMasterParam.clockPolarity = USCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;//
    USCI_A_SPI_initMaster(USCI_A0_BASE, &spiMasterParam);
    // Enable USCI_A0
    USCI_A_SPI_enable(USCI_A0_BASE);

    // DMA0
    DMA_initParam dma0Param = {0};
    dma0Param.channelSelect = DMA_CHANNEL_0;
    dma0Param.transferModeSelect = DMA_TRANSFER_SINGLE;
    dma0Param.transferSize = 97;
    dma0Param.triggerSourceSelect = DMA_TRIGGERSOURCE_17;   // UCA0TXIFG
    dma0Param.transferUnitSelect = DMA_SIZE_SRCBYTE_DSTBYTE;
    dma0Param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
    DMA_init(&dma0Param);
    // DMA0 source address
    DMA_setSrcAddress(
        DMA_CHANNEL_0,
        (uint32_t) chip0.getControlData(),
        DMA_DIRECTION_INCREMENT
        );
    // DMA0 destination address
    DMA_setDstAddress(
        DMA_CHANNEL_0,
        USCI_A_SPI_getTransmitBufferAddressForDMA(USCI_A0_BASE),
        DMA_DIRECTION_UNCHANGED
        );

    // Timer_A0
    Timer_A_initUpModeParam ta0Param = {0};
    ta0Param.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    ta0Param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    ta0Param.timerPeriod = 8192;
    ta0Param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
    ta0Param.startTimer = true;
    Timer_A_initUpMode(TIMER_A0_BASE, &ta0Param);
}

void LedController::appSetup(){
    chip0.setAllLED(0, 65535);
    chip0.setAllLED(1, 65535);
    chip0.setAllLED(2, 65535);
    chip0.getControlData()->DSPRPT = 1;
    for(int i = 0 ;i < 21;++i)
        chip0.getControlData()->DOTCOR[i] = 0xFFFF;
}

void LedController::start(){

}

void LedController::stop(){

}

TLC5955 *LedController::getTLCModule(int n){

}

void latch(){

    GPIO_setOutputHighOnPin(
        GPIO_PORT_P4,
        GPIO_PIN1
        );
    GPIO_setOutputHighOnPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );
    for(volatile int i = 0;i < 32; ++i);
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P4,
        GPIO_PIN1
        );
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );
}

__attribute__((__interrupt__(TIMER0_A1_VECTOR)))
void TA0CCR_TA0IFG_ISR(void) {
    switch (__even_in_range(TA0IV, 0x0E)) {
        case 0x0E:  // TA0CTL TAIFG
            if(writeControl != 0)latch();

            if(writeControl == 2){
                LedController::chip0.setAllLED(0, LedController::globalLight);
                LedController::chip0.setAllLED(1, LedController::globalLight);
                LedController::chip0.setAllLED(2, LedController::globalLight);
                DMA_setSrcAddress(
                                  DMA_CHANNEL_0,
                                  (uint32_t) LedController::chip0.getGSData(),
                                  DMA_DIRECTION_INCREMENT
                                 );
                LedController::globalLight += 0x1000;
            }else{
                ++writeControl;
            }

            // Enable DMA0
            DMA_enableTransfers(DMA_CHANNEL_0);
            DMA_enableInterrupt(DMA_CHANNEL_0);

            // Rising edge of USCI_A0 transmit flag
            if (USCI_A_SPI_getInterruptStatus(
                    USCI_A0_BASE,
                    USCI_A_SPI_TRANSMIT_INTERRUPT
                    )) {
                // Clear
                USCI_A_SPI_clearInterrupt(
                    USCI_A0_BASE,
                    USCI_A_SPI_TRANSMIT_INTERRUPT
                    );
                // Set
                UCA0IFG |= UCTXIFG;
            }

            break;
    }
}

__attribute__((__interrupt__(DMA_VECTOR)))
void DMA_ISR(void) {
    switch (__even_in_range(DMAIV, 0x06)) {
        case 0x00:  // DMA0IFG

            break;
    }
}
