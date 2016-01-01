#include "driverlib.h"

char TxString[10];

int test(void) {
    // Stop WDT
    WDT_A_hold(WDT_A_BASE);

    // UCA0SIMO(P3.3), UCA0SOMI(P3.4)
    GPIO_setAsPeripheralModuleFunctionInputPin(// out?
        GPIO_PORT_P3,
        GPIO_PIN3 | GPIO_PIN4
        );
    // UCA0CLK(P2.7)
    GPIO_setAsPeripheralModuleFunctionInputPin(// out?
        GPIO_PORT_P2,
        GPIO_PIN7
        );

    GPIO_setAsOutputPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );

    // Initialize SPI Master
    USCI_A_SPI_initMasterParam spiMasterParam = {0};
    spiMasterParam.selectClockSource = USCI_A_SPI_CLOCKSOURCE_SMCLK;
    spiMasterParam.clockSourceFrequency = UCS_getSMCLK();
    spiMasterParam.desiredSpiClock = 1000000;
    spiMasterParam.msbFirst = USCI_A_SPI_MSB_FIRST;
    spiMasterParam.clockPhase = USCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;//
    spiMasterParam.clockPolarity = USCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH;//
    USCI_A_SPI_initMaster(USCI_A0_BASE, &spiMasterParam);
    // Enable SPI
    USCI_A_SPI_enable(USCI_A0_BASE);
/*
    while(!USCI_A_SPI_getInterruptStatus(USCI_A0_BASE, UCTXIFG))
        ;
*/
    // DMA
    DMA_initParam dma0Param = {0};
    dma0Param.channelSelect = DMA_CHANNEL_0;
    dma0Param.transferModeSelect = DMA_TRANSFER_SINGLE;
    dma0Param.transferSize = 10;
    dma0Param.triggerSourceSelect = DMA_TRIGGERSOURCE_17;   // UCA0TXIFG
    dma0Param.transferUnitSelect = DMA_SIZE_SRCBYTE_DSTBYTE;
    dma0Param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
    DMA_init(&dma0Param);

    DMA_setSrcAddress(
        DMA_CHANNEL_0,
        (uint32_t) TxString,
        DMA_DIRECTION_INCREMENT
        );
    DMA_setDstAddress(
        DMA_CHANNEL_0,
        USCI_A_SPI_getTransmitBufferAddressForDMA(USCI_A0_BASE),
        DMA_DIRECTION_UNCHANGED
        );

    // Timer_A
    Timer_A_initUpModeParam ta0Param = {0};
    ta0Param.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    ta0Param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    ta0Param.timerPeriod = 32767;
    ta0Param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
    ta0Param.startTimer = true;
    Timer_A_initUpMode(TIMER_A0_BASE, &ta0Param);

    // LPM0
    __bis_SR_register(LPM0_bits + GIE);

    while (1);

    return 0;
}

__attribute__ (( __interrupt__ (TIMER0_A1_VECTOR)))
void TA0CCR_TA0IFG_ISR(void) {
    switch (__even_in_range(TA0IV, 0x0E)) {
        case 0x0E:  // TA0CTL TAIFG
            for (int i = 0; i < 10; ++i)
                TxString[i] = i;

            DMA_enableTransfers(DMA_CHANNEL_0);
            GPIO_toggleOutputOnPin(
               GPIO_PORT_P1,
               GPIO_PIN0
              );
            break;
    }
}
