#include "driverlib.h"
#include "nRF24L01+.h"

uint8_t TxData[64] = {0};
uint8_t RxData[64] = {0};

int b(void) {
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
    // P2.1
    GPIO_setAsInputPinWithPullUpResistor(
        GPIO_PORT_P2,
        GPIO_PIN1
        );
    GPIO_selectInterruptEdge(
        GPIO_PORT_P2,
        GPIO_PIN1,
        GPIO_HIGH_TO_LOW_TRANSITION
        );
    GPIO_enableInterrupt(
        GPIO_PORT_P2,
        GPIO_PIN1
        );
    // P2.5 (nRF24L01+ CSN)
    GPIO_setAsOutputPin(
        GPIO_PORT_P2,
        GPIO_PIN5
        );
    GPIO_setOutputHighOnPin(
        GPIO_PORT_P2,
        GPIO_PIN5
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
    dma0Param.transferSize = 2;     // W_REGISTER(CONFIG), PWR_UP & (~PRIM_RX)
    dma0Param.triggerSourceSelect = DMA_TRIGGERSOURCE_17;   // UCA0TXIFG
    dma0Param.transferUnitSelect = DMA_SIZE_SRCBYTE_DSTBYTE;
    dma0Param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
    DMA_init(&dma0Param);
    // DMA0 source address
    DMA_setSrcAddress(
        DMA_CHANNEL_0,
        (uint32_t) TxData,
        DMA_DIRECTION_INCREMENT
        );
    // DMA0 destination address
    DMA_setDstAddress(
        DMA_CHANNEL_0,
        USCI_A_SPI_getTransmitBufferAddressForDMA(USCI_A0_BASE),
        DMA_DIRECTION_UNCHANGED
        );
    // Enable DMA0 interrupt
    DMA_enableInterrupt(DMA_CHANNEL_0);



    // Modify Data
    TxData[0] = W_REGISTER(CONFIG);
    TxData[1] = PWR_UP & (~PRIM_RX);

    // Enable DMA0
    DMA_enableTransfers(DMA_CHANNEL_0);

    // nRF24L01+ CSN High to Low
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P2,
        GPIO_PIN5
        );

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

    // Low power mode 0 and general interrupt enable
    __bis_SR_register(LPM0_bits + GIE);

    return 0;
}

__attribute__((__interrupt__(PORT2_VECTOR)))
void P2_ISR(void) {
    switch (__even_in_range(P2IV, 0x10)) {
        case 0x04:  // P2IFG.1
            // Disable DMA0
            DMA_disableTransfers(DMA_CHANNEL_0);
            // DMA0
            DMA_initParam dma0Param = {0};
            dma0Param.channelSelect = DMA_CHANNEL_0;
            dma0Param.transferModeSelect = DMA_TRANSFER_SINGLE;
            dma0Param.transferSize = 3;     // W_TX_PAYLOAD, Byte0, Byte1
            dma0Param.triggerSourceSelect = DMA_TRIGGERSOURCE_17;   // UCA0TXIFG
            dma0Param.transferUnitSelect = DMA_SIZE_SRCBYTE_DSTBYTE;
            dma0Param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
            DMA_init(&dma0Param);
            // DMA0 source address
            DMA_setSrcAddress(
                DMA_CHANNEL_0,
                (uint32_t) TxData,
                DMA_DIRECTION_INCREMENT
                );
            // DMA0 destination address
            DMA_setDstAddress(
                DMA_CHANNEL_0,
                USCI_A_SPI_getTransmitBufferAddressForDMA(USCI_A0_BASE),
                DMA_DIRECTION_UNCHANGED
                );
            // Enable DMA0 interrupt
            DMA_enableInterrupt(DMA_CHANNEL_0);



            // Modify Data
            TxData[0] = W_TX_PAYLOAD;
            TxData[1] = 3;
            TxData[2] = 7;

            // Enable DMA0
            DMA_enableTransfers(DMA_CHANNEL_0);

            // nRF24L01+ CSN High to Low
            GPIO_setOutputLowOnPin(
                GPIO_PORT_P2,
                GPIO_PIN5
                );

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
        case 0x02:  // DMA0IFG
            // nRF24L01+ CSN Low to High
            GPIO_setOutputHighOnPin(
                GPIO_PORT_P2,
                GPIO_PIN5
                );

            // Toggle LED1
            GPIO_toggleOutputOnPin(
                GPIO_PORT_P1,
                GPIO_PIN0
                );

            break;
    }
}
