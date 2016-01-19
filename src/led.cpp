#include "led.hpp"
#include "misc.hpp"
#include "driverlib.h"
#include "tlc5955.hpp"

uint16_t LedController::globalLight = 0xFFFF;
TLC5955 LedController::chip[2];
static int writeControl = 0;
static int start = 0;

void LedController::init()
{
    // chip 0
    // P3.3 (UCA0SIMO)
    P3SEL |= GPIO_PIN3;
    // P2.7 (UCA0CLK)
    P2SEL |= GPIO_PIN7;

    // chip 1
    PMAPKEYID = 0x2D52;
    // P4.3 (UCA1SIMO)
    P4MAP3 = PM_UCA1SIMO;
    P4SEL |= GPIO_PIN3;
    // P4.0 (UCA1CLK)
    P4MAP0 = PM_UCA1CLK;
    P4SEL |= GPIO_PIN0;
    PMAPKEYID = 0x0402; /* lock again */

    // SMCLK as GSCLK
    P2SEL |= GPIO_PIN2;
    P2DIR |= GPIO_PIN2;

    // chip0: USCI_A0 (SPI Master)
    USCI_A_SPI_initMasterParam spiMasterParam = {0};
    spiMasterParam.selectClockSource = USCI_A_SPI_CLOCKSOURCE_SMCLK;
    spiMasterParam.clockSourceFrequency = UCS_getSMCLK();
    spiMasterParam.desiredSpiClock = 1000000;
    spiMasterParam.msbFirst = USCI_A_SPI_MSB_FIRST;
    spiMasterParam.clockPhase =
        USCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;              //
    spiMasterParam.clockPolarity = USCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;  //
    USCI_A_SPI_initMaster(USCI_A0_BASE, &spiMasterParam);
    // Enable USCI_A0
    USCI_A_SPI_enable(USCI_A0_BASE);

    // chip1: USCI_A1 (SPI Master)
    USCI_A_SPI_initMasterParam spiMasterParam1 = {0};
    spiMasterParam1.selectClockSource = USCI_A_SPI_CLOCKSOURCE_SMCLK;
    spiMasterParam1.clockSourceFrequency = UCS_getSMCLK();
    spiMasterParam1.desiredSpiClock = 1000000;
    spiMasterParam1.msbFirst = USCI_A_SPI_MSB_FIRST;
    spiMasterParam1.clockPhase =
        USCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;              //
    spiMasterParam1.clockPolarity = USCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;  //
    USCI_A_SPI_initMaster(USCI_A1_BASE, &spiMasterParam1);
    // Enable USCI_A1
    USCI_A_SPI_enable(USCI_A1_BASE);

    // chip0: DMA0
    DMA_initParam dma0Param = {0};
    dma0Param.channelSelect = DMA_CHANNEL_0;
    dma0Param.transferModeSelect = DMA_TRANSFER_SINGLE;
    dma0Param.transferSize = 97;
    dma0Param.triggerSourceSelect = DMA_TRIGGERSOURCE_17;  // UCA0TXIFG
    dma0Param.transferUnitSelect = DMA_SIZE_SRCBYTE_DSTBYTE;
    dma0Param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
    DMA_init(&dma0Param);
    // DMA0 source address
    DMA_setSrcAddress(DMA_CHANNEL_0, (uint32_t)chip[0].getControlData(),
                      DMA_DIRECTION_INCREMENT);
    // DMA0 destination address
    DMA_setDstAddress(DMA_CHANNEL_0,
                      USCI_A_SPI_getTransmitBufferAddressForDMA(USCI_A0_BASE),
                      DMA_DIRECTION_UNCHANGED);

    // chip1: DMA1
    DMA_initParam dma1Param = {0};
    dma1Param.channelSelect = DMA_CHANNEL_1;
    dma1Param.transferModeSelect = DMA_TRANSFER_SINGLE;
    dma1Param.transferSize = 97;
    dma1Param.triggerSourceSelect = DMA_TRIGGERSOURCE_21;  // UCA1TXIFG
    dma1Param.transferUnitSelect = DMA_SIZE_SRCBYTE_DSTBYTE;
    dma1Param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
    DMA_init(&dma1Param);
    // DMA1 source address
    DMA_setSrcAddress(DMA_CHANNEL_1, (uint32_t)chip[1].getControlData(),
                      DMA_DIRECTION_INCREMENT);
    // DMA1 destination address
    DMA_setDstAddress(DMA_CHANNEL_1,
                      USCI_A_SPI_getTransmitBufferAddressForDMA(USCI_A1_BASE),
                      DMA_DIRECTION_UNCHANGED);

    //const uint16_t period = 179;
    //const uint16_t factor = 64;
    const uint16_t period = 862;
    const uint16_t factor = 64;
    // Timer output mode set/reset for lat signal
    Timer_A_outputPWMParam ta1PwmParam;
    ta1PwmParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    ta1PwmParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    ta1PwmParam.timerPeriod = period;
    ta1PwmParam.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    ta1PwmParam.compareOutputMode = TIMER_A_OUTPUTMODE_SET_RESET;
    ta1PwmParam.dutyCycle = period - period / factor;
    Timer_A_outputPWM(TIMER_A0_BASE, &ta1PwmParam);

    // Timer_A0 for data output
    Timer_A_initUpModeParam ta0Param = {0};
    ta0Param.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    ta0Param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    ta0Param.timerPeriod = period;
    ta0Param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
    ta0Param.startTimer = true;
    Timer_A_initUpMode(TIMER_A0_BASE, &ta0Param);
}

void LedController::appSetup()
{
    for(int chipid = 0; chipid < 2;++chipid){
        chip[chipid].setAllLED(0, 500);
        chip[chipid].setAllLED(1, 500);
        chip[chipid].setAllLED(2, 500);
        chip[chipid].getControlData()->ESPWM = 1;
        chip[chipid].getControlData()->TWGRST= 1;
        chip[chipid].getControlData()->DSPRPT = 1;
        chip[chipid].getControlData()->BC_R = 0x7F;
        chip[chipid].getControlData()->BC_G = 0x4F;
        chip[chipid].getControlData()->BC_B = 0x1F;
        chip[chipid].getControlData()->MC_R = 0x3;
        chip[chipid].getControlData()->MC_G = 0x3;
        chip[chipid].getControlData()->MC_B = 0x3;
        for (int i = 0; i < 21; ++i)
            chip[chipid].getControlData()->DOTCOR[i] = 0xFFFF;
    }
}

void LedController::start()
{
    ::start = 1;
    writeControl = 0;
    // P1.2 (LAT)
    P1SEL |= GPIO_PIN2;
    P1DIR |= GPIO_PIN2;
}

void LedController::stop()
{
    ::start = 0;
    // P1.2 (LAT)
    P1SEL &= ~GPIO_PIN2;
    P1DIR &= ~GPIO_PIN2;
}

TLC5955 *LedController::getTLCModule(int n)
{
    switch (n) {
        case 0:
        case 1:
            return &chip[n];
        default: return &chip[0];
    }
}

uint8_t mode = 0;
uint8_t id = 0;
uint8_t idcount = 1;

void LedController::switchid(){
    id++;
    if(id >= idcount) id = 0;
}

uint16_t time = 0;

__attribute__((__interrupt__(TIMER0_A1_VECTOR))) void TA0CCR_TA0IFG_ISR(void)
{
    switch (__even_in_range(TA0IV, 0x0E)) {
        case 0x0E:  // TA0CTL TAIFG
            if (::start) {
                if (writeControl == 2) {
                    time++;
                    getImgRange(
                                mode,
                                id,
                                time,
                                LedController::getTLCModule(0)->getGSData(),
                                0,
                                16
                               );
                    getImgRange(
                                mode,
                                id,
                                time,
                                LedController::getTLCModule(1)->getGSData(),
                                17,
                                32
                               );

                    DMA_setSrcAddress(
                        DMA_CHANNEL_0,
                        (uint32_t)LedController::chip[0].getGSDataCommand(),
                        DMA_DIRECTION_INCREMENT);
                    DMA_setSrcAddress(
                        DMA_CHANNEL_1,
                        (uint32_t)LedController::chip[1].getGSDataCommand(),
                        DMA_DIRECTION_INCREMENT);
                } else {
                    ++writeControl;
                }

                // Enable DMA0
                DMA_enableTransfers(DMA_CHANNEL_0);
                DMA_enableInterrupt(DMA_CHANNEL_0);

                // Enable DMA0
                DMA_enableTransfers(DMA_CHANNEL_1);
                DMA_enableInterrupt(DMA_CHANNEL_1);

                // Rising edge of USCI_A0 transmit flag
                if (USCI_A_SPI_getInterruptStatus(
                        USCI_A0_BASE, USCI_A_SPI_TRANSMIT_INTERRUPT)) {
                    // Clear
                    USCI_A_SPI_clearInterrupt(USCI_A0_BASE,
                                              USCI_A_SPI_TRANSMIT_INTERRUPT);
                    // Set
                    UCA0IFG |= UCTXIFG;
                }
                // Rising edge of USCI_A0 transmit flag
                if (USCI_A_SPI_getInterruptStatus(
                        USCI_A1_BASE, USCI_A_SPI_TRANSMIT_INTERRUPT)) {
                    // Clear
                    USCI_A_SPI_clearInterrupt(USCI_A1_BASE,
                                              USCI_A_SPI_TRANSMIT_INTERRUPT);
                    // Set
                    UCA1IFG |= UCTXIFG;
                }
            }
            break;
    }
//    __bic_SR_register_on_exit(LPM0_bits); /* Back to main loop */
}

__attribute__((__interrupt__(DMA_VECTOR))) void DMA_ISR(void)
{
    switch (__even_in_range(DMAIV, 0x06)) {
        default: {}/* nothing to do atm */
    }
}

