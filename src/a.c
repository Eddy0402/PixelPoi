#include "driverlib.h"

char TxString[10] = {0};
char RxString[10] = {0};

int a(void) {
    // Stop WDT_A
    WDT_A_hold(WDT_A_BASE);

    // P3.3 (UCA0SIMO)
    GPIO_setAsPeripheralModuleFunctionInputPin(// in or out?
        GPIO_PORT_P3,
        GPIO_PIN3
        );
    // P2.7 (UCA0CLK)
    GPIO_setAsPeripheralModuleFunctionInputPin(// in or out?
        GPIO_PORT_P2,
        GPIO_PIN7
        );
    // P1.0 (LED1)
    GPIO_setAsOutputPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );

    // USCI_A0 (SPI Master)
    USCI_A_SPI_initMasterParam spiMasterParam = {0};
    spiMasterParam.selectClockSource = USCI_A_SPI_CLOCKSOURCE_SMCLK;
    spiMasterParam.clockSourceFrequency = UCS_getSMCLK();
    spiMasterParam.desiredSpiClock = 1000000;
    spiMasterParam.msbFirst = USCI_A_SPI_MSB_FIRST;
    spiMasterParam.clockPhase = USCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;//
    spiMasterParam.clockPolarity = USCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH;//
    USCI_A_SPI_initMaster(USCI_A0_BASE, &spiMasterParam);
    // Enable USCI_A0
    USCI_A_SPI_enable(USCI_A0_BASE);

    // DMA0
    DMA_initParam dma0Param = {0};
    dma0Param.channelSelect = DMA_CHANNEL_0;
    dma0Param.transferModeSelect = DMA_TRANSFER_SINGLE;
    dma0Param.transferSize = 10;
    dma0Param.triggerSourceSelect = DMA_TRIGGERSOURCE_17;   // UCA0TXIFG
    dma0Param.transferUnitSelect = DMA_SIZE_SRCBYTE_DSTBYTE;
    dma0Param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
    DMA_init(&dma0Param);
    // DMA0 source address
    DMA_setSrcAddress(
        DMA_CHANNEL_0,
        (uint32_t) TxString,
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
    ta0Param.timerPeriod = 32767;
    ta0Param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
    ta0Param.startTimer = true;
    Timer_A_initUpMode(TIMER_A0_BASE, &ta0Param);





// Slave

    // P3.0 (UCB0SIMO), P3.2 (UCB0CLK)
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P3,
        GPIO_PIN0 | GPIO_PIN2
        );
    // P4.7 (LED2)
    GPIO_setAsOutputPin(
        GPIO_PORT_P4,
        GPIO_PIN7
        );
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P4,
        GPIO_PIN7
        );

    // USCI_B0 (SPI Slave)
    USCI_B_SPI_initSlave(
        USCI_B0_BASE,
        USCI_B_SPI_MSB_FIRST,
        USCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT,
        USCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
        );
    // Enable USCI_B0
    USCI_B_SPI_enable(USCI_B0_BASE);

    // DMA1
    DMA_initParam dma1Param = {0};
    dma1Param.channelSelect = DMA_CHANNEL_1;
    dma1Param.transferModeSelect = DMA_TRANSFER_REPEATED_SINGLE;
    dma1Param.transferSize = 10;
    dma1Param.triggerSourceSelect = DMA_TRIGGERSOURCE_18;   // UCB0RXIFG
    dma1Param.transferUnitSelect = DMA_SIZE_SRCBYTE_DSTBYTE;
    dma1Param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
    DMA_init(&dma1Param);
    // DMA1 source address
    DMA_setSrcAddress(
        DMA_CHANNEL_1,
        USCI_B_SPI_getReceiveBufferAddressForDMA(USCI_B0_BASE),
        DMA_DIRECTION_UNCHANGED
        );
    // DMA1 destination address
    DMA_setDstAddress(
        DMA_CHANNEL_1,
        (uint32_t) RxString,
        DMA_DIRECTION_INCREMENT
        );
    // Enable DMA1
    DMA_enableTransfers(DMA_CHANNEL_1);
    // Enable DMA1 interrupt
    DMA_enableInterrupt(DMA_CHANNEL_1);

// Slave





    // Low power mode 0 and general interrupt enable
    __bis_SR_register(LPM0_bits + GIE);

    return 0;
}
/*
__attribute__((__interrupt__(TIMER0_A1_VECTOR)))
void TA0CCR_TA0IFG_ISR(void) {
    switch (__even_in_range(TA0IV, 0x0E)) {
        case 0x0E:  // TA0CTL TAIFG
            // Modify data
            for (int i = 0; i < 10; ++i)
                TxString[i] += i;

            // Enable DMA0
            DMA_enableTransfers(DMA_CHANNEL_0);

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

            GPIO_toggleOutputOnPin(
                GPIO_PORT_P1,
                GPIO_PIN0
                );

            break;
    }
}

__attribute__((__interrupt__(DMA_VECTOR)))
void DMA_ISR(void) {
    switch (__even_in_range(DMAIV, 0x06)) {
        case 0x04:  // DMA1IFG
            if (TxString[1] == RxString[1]
                    && TxString[2] == RxString[2]
                    && TxString[3] == RxString[3]
                    && TxString[4] == RxString[4]
                    && TxString[5] == RxString[5]
                    && TxString[6] == RxString[6]
                    && TxString[7] == RxString[7]
                    && TxString[8] == RxString[8]
                    && TxString[9] == RxString[9])
                GPIO_toggleOutputOnPin(
                    GPIO_PORT_P4,
                    GPIO_PIN7
                    );

            break;
    }
}
*/
