//******************************************************************************
//!
//! DMA - Single Transfer using SPI, TX on DMA Channel 0, RX on DMA Channel 1
//!
//!  This is the SPI Master.
//!  This code has to be used with USCI_A_SPI_slave.c (slave SPI). DMA channel 0 is
//!  used to transfer a single byte while DMA channel 1 is used to RX from slave
//!  SPI at the same time. This code will set P1.0 if RX character is correct
//!  and clears P1.0 if received character is wrong. Watchdog in interval mode
//!  triggers block transfer every 1000ms.
//!  ACLK = REFO = 32kHz, MCLK = SMCLK = default DCO 1048576Hz
//!
//!                MSP430F552x
//!             -----------------
//!         /|\|              XIN|-
//!          | |                 | 32kHz
//!          --|RST          XOUT|-
//!            |                 |
//!            |             P1.0|-> LED
//!            |    P3.3/UCA0SIMO|-> Data Out
//!            |    P3.4/UCA0SOMI|<- Data In
//!            |     P2.7/UCA0CLK|-> Serial Clock Out
//!            |                 |
//!
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - DMA peripheral
//! - SPI peripheral
//! - GPIO Port peripheral
//! - UCA0SIMO
//! - UCA0SOMI
//! - UCA0CLK
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - WDT_A_VECTOR
//!
//******************************************************************************

#include "driverlib.h"

char TxString[10];
char RxString[10];
uint16_t dmaEnableWord;


int test(void)
{
    //Stop WDT
    WDT_A_hold(WDT_A_BASE);

    //P3.4,3 & P2.7 option select
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P3,
        GPIO_PIN4 + GPIO_PIN3
        );
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P2,
        GPIO_PIN7
        );

    //Initialize Master
    USCI_A_SPI_initMasterParam param = {0};
    param.selectClockSource = USCI_A_SPI_CLOCKSOURCE_ACLK;
    param.clockSourceFrequency = UCS_getACLK();
    param.desiredSpiClock = 1000000;
    param.msbFirst = USCI_A_SPI_MSB_FIRST;
    param.clockPhase = USCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
    param.clockPolarity = USCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH;
    USCI_A_SPI_initMaster(USCI_A0_BASE, &param);

    //Enable SPI module
    USCI_A_SPI_enable(USCI_A0_BASE);

    //USCI_A0 TX buffer ready?
    while(!USCI_A_SPI_getInterruptStatus(USCI_A0_BASE, UCTXIFG))
		;

    //Initialize and Setup DMA Channel 0
    /*
     * Configure DMA channel 0
     * Configure channel for single transfer
     * DMA transfers will be disabled and interrupt flag will be set after every
     *   1 transfer
     * Use DMA Trigger Source 17 (UCA0TXIFG)
     * Transfer Byte-to-byte
     * Trigger transfer on signal held high
     */
    DMA_initParam param0 = {0};
    param0.channelSelect = DMA_CHANNEL_0;
    param0.transferModeSelect = DMA_TRANSFER_BLOCK;
    param0.transferSize = 10;
    param0.triggerSourceSelect = DMA_TRIGGERSOURCE_17;
    param0.transferUnitSelect = DMA_SIZE_SRCBYTE_DSTBYTE;
    //param0.triggerTypeSelect = DMA_TRIGGER_HIGH;
    DMA_init(&param0);
    /*
     * Configure DMA channel 0
     * Use TxString as source
     * Increment source address after every transfer
     */
    DMA_setSrcAddress(DMA_CHANNEL_0,
                      (uint32_t)TxString,
                      DMA_DIRECTION_INCREMENT);
    /*
     * Configure DMA channel 0
     * Use SPI TX Buffer as destination
     * Don't move the destination address after every transfer
     */
    DMA_setDstAddress(DMA_CHANNEL_0,
                      USCI_A_SPI_getTransmitBufferAddressForDMA(USCI_A0_BASE),
                      DMA_DIRECTION_UNCHANGED);

    //Initialize and Setup DMA Channel 1
    /*
     * Configure DMA channel 1
     * Configure channel for single transfer
     * DMA transfers will be disabled and interrupt flag will be set after every
     *   1 transfer
     * Use DMA Trigger Source 16 (UCA0RXIFG)
     * Transfer Byte-to-byte
     * Trigger transfer on signal held high
     */

    DMA_initParam param1 = {0};
    param1.channelSelect = DMA_CHANNEL_1;
    param1.transferModeSelect = DMA_TRANSFER_REPEATED_SINGLE;
    param1.transferSize = 1;
    param1.triggerSourceSelect = DMA_TRIGGERSOURCE_16;
    param1.transferUnitSelect = DMA_SIZE_SRCWORD_DSTWORD;
    param1.triggerTypeSelect = DMA_TRIGGER_HIGH;
    DMA_init(&param1);

    /*
     * Configure DMA channel 1
     * Use SPI RX Buffer as source
     * Don't move the source address after every transfer
     */
	dmaEnableWord = DMA0CTL | DMAEN;
    DMA_setSrcAddress(DMA_CHANNEL_1,
					  //(uint32_t) &dmaEnableWord,
					  USCI_A_SPI_getReceiveBufferAddressForDMA(USCI_A0_BASE),
                      DMA_DIRECTION_UNCHANGED);
    /*
     * Configure DMA channel 1
     * Use RxString as destination
     * Increment destination address after every transfer
     */
    DMA_setDstAddress(DMA_CHANNEL_1,
                      //(uint32_t)DMA0CTL,
                      (uint32_t)&RxString,
                      DMA_DIRECTION_UNCHANGED);

    //Clear TxString && RxString
	for(int i = 0;i < 10;++i){
    	TxString[i] = i;
	}

    //Enable DMA transfers on channel 1
    DMA_enableTransfers(DMA_CHANNEL_1);

    //Enable DMA transfers on channel 0
    DMA_enableTransfers(DMA_CHANNEL_0);


    Timer_A_initUpModeParam ta0Param = {0};
    ta0Param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    ta0Param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    ta0Param.timerPeriod = 32767;
    ta0Param.startTimer = true;
    Timer_A_initUpMode(TIMER_A0_BASE, &ta0Param);


    //Enter LPM3 w/ interrupts
    __bis_SR_register(LPM3_bits + GIE);
    //For Debugger
    __no_operation();
	while(1);
}

#pragma vector = TIMER_A
