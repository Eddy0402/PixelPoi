#include "nrf24l01p.hpp"

void nrf24l01pInitSPI(void) {
    // P3.0 (UCB0SIMO), P3.1 (UCBSOMI), P3.2 (UCB0CLK)
    P3SEL |= GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2;

    // USCI_B0 (SPI Master)
    USCI_B_SPI_initMasterParam spiMasterParam = {0};
    spiMasterParam.selectClockSource = USCI_B_SPI_CLOCKSOURCE_SMCLK;
    spiMasterParam.clockSourceFrequency = UCS_getSMCLK();
    spiMasterParam.desiredSpiClock = 1000000;
    spiMasterParam.msbFirst = USCI_B_SPI_MSB_FIRST;
    spiMasterParam.clockPhase = USCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;//
    spiMasterParam.clockPolarity = USCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH;//
    USCI_B_SPI_initMaster(USCI_B0_BASE, &spiMasterParam);
    // Enable USCI_B0
    USCI_B_SPI_enable(USCI_B0_BASE);
}

void nrf24l01pSetAsCE(uint8_t port, uint16_t pin) {
    portCE = port;
    pinCE = pin;
}
void nrf24l01pSetLowOnCE(void) {
    GPIO_setAsOutputPin(portCE, pinCE);
    GPIO_setOutputLowOnPin(portCE, pinCE);
}

void nrf24l01pSetHighOnCE(void) {
    GPIO_setAsOutputPin(portCE, pinCE);
    GPIO_setOutputHighOnPin(portCE, pinCE);
}

void nrf24l01pSetAsCSN(uint8_t port, uint16_t pin) {
    portCSN = port;
    pinCSN = pin;
}
void nrf24l01pSetLowOnCSN(void) {
    GPIO_setAsOutputPin(portCSN, pinCSN);
    GPIO_setOutputLowOnPin(portCSN, pinCSN);
}
void nrf24l01pSetHighOnCSN(void) {
    GPIO_setAsOutputPin(portCSN, pinCSN);
    GPIO_setOutputHighOnPin(portCSN, pinCSN);
}

uint8_t nrf24l01pReadRegister(uint8_t reg) {
    nrf24l01pSetLowOnCSN();

    USCI_B_SPI_transmitData(USCI_B0_BASE, R_REGISTER(reg));
    while (!USCI_B_SPI_getInterruptStatus(USCI_B0_BASE, USCI_B_SPI_RECEIVE_INTERRUPT));
    USCI_B_SPI_clearInterrupt(USCI_B0_BASE, USCI_B_SPI_RECEIVE_INTERRUPT);

    USCI_B_SPI_transmitData(USCI_B0_BASE, 0);
    while (!USCI_B_SPI_getInterruptStatus(USCI_B0_BASE, USCI_B_SPI_RECEIVE_INTERRUPT));
    uint8_t bits = USCI_B_SPI_receiveData(USCI_B0_BASE);

    nrf24l01pSetHighOnCSN();

	return bits;
}
void nrf24l01pWriteRegister(uint8_t reg, uint8_t bits) {
    nrf24l01pSetLowOnCSN();

    USCI_B_SPI_transmitData(USCI_B0_BASE, W_REGISTER(reg));
    while (!USCI_B_SPI_getInterruptStatus(USCI_B0_BASE, USCI_B_SPI_RECEIVE_INTERRUPT));
    USCI_B_SPI_clearInterrupt(USCI_B0_BASE, USCI_B_SPI_RECEIVE_INTERRUPT);

    USCI_B_SPI_transmitData(USCI_B0_BASE, bits);
    while (!USCI_B_SPI_getInterruptStatus(USCI_B0_BASE, USCI_B_SPI_RECEIVE_INTERRUPT));

    nrf24l01pSetHighOnCSN();
}

void nrf24l01pPowerDown(void) {
	// Read CONFIG
	uint8_t config = nrf24l01pReadRegister(CONFIG);

    // Power down
    config &= ~PWR_UP;

    // Write CONFIG
	nrf24l01pWriteRegister(CONFIG, config);
}
void nrf24l01pPowerUp(void) {
	// Read CONFIG
	uint8_t config = nrf24l01pReadRegister(CONFIG);

    // Power up
    config |= PWR_UP;

    // Write CONFIG
	nrf24l01pWriteRegister(CONFIG, config);
}

void nrf24l01pSetRX(void) {
	// Read CONFIG
	uint8_t config = nrf24l01pReadRegister(CONFIG);

    // RX
    config |= PRIM_RX;

    // Write CONFIG
	nrf24l01pWriteRegister(CONFIG, config);
}
void nrf24l01pSetTX(void) {
	// Read CONFIG
	uint8_t config = nrf24l01pReadRegister(CONFIG);

    // TX
    config &= ~PRIM_RX;

    // Write CONFIG
	nrf24l01pWriteRegister(CONFIG, config);
}
/*
__attribute__((__interrupt__(USCI_B0_VECTOR)))
void UCB0_ISR(void) {
    switch (__even_in_range(UCB0IV, 0x04)) {
        case 0x02:  // UCB0RXIFG
            
            break;
    }
}
*/
