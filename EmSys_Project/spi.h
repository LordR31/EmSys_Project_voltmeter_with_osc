#ifndef SPI_H_
#define SPI_H_

#include <avr/io.h>

// SPI pins
#define SPI_DDR   DDRB
#define SPI_PORT  PORTB
#define SPI_MOSI  PB2
#define SPI_MISO  PB3
#define SPI_SCK   PB1

void SPI_init(void);
uint8_t SPI_transfer(uint8_t data);

#endif /* SPI_H_ */
