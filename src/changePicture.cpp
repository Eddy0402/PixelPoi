#include "driverlib.h"
#include "nrf24l01p.hpp"

void changePicture(void) {
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN7);

	nrf24l01pInitSPI();
	nrf24l01pSetAsCE(GPIO_PORT_P2, GPIO_PIN4);
	nrf24l01pSetAsCSN(GPIO_PORT_P2, GPIO_PIN5);
	nrf24l01pSetHighOnCSN();

	nrf24l01pPowerUp();

	uint8_t config = nrf24l01pReadRegister(CONFIG);	

	if (config == 0x08)
    	GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN7);
}
