#include "spi.h"

void SPI_init(void) {
	SPI_DDR |= (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << PB0);    // MISO, SCK & SS output (SS unused)
	PORTB |= (1 << PB0);
	SPI_DDR &= ~(1 << PB3);                                      // MISO  input
	PORTB &= ~(1 << PB3);                                        // pull-up is OFF
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPOL) | (1 << CPHA); // enable spi and set master
	SPSR |= (1 << SPI2X);                                        // double speed
}

uint8_t SPI_transfer(uint8_t data) {
	SPDR = data;                                                 // put the data inside SPDR
	while (!(SPSR & (1 << SPIF)));                               // wait for it to send
	return SPDR;                                                 // return SPDR
}

void Touch_SPI_Init(void) {
	DDRB |= (1 << PB7);                                          // set PB7 as output (CS_TOUCH)
	PORTB |= (1 << PB7);                                         // set CS_TOUCH high (inactive)
}
