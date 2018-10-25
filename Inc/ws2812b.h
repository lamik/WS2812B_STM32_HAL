/*
 * ws2812b.h
 *
 *	The MIT License.
 *	Created on: 14.07.2017
 *		Author: Mateusz Salamon
 *		www.msalamon.pl
 *		mateusz@msalamon.pl
 */

#ifndef WS2812B_H_
#define WS2812B_H_

// For 6 MHz SPI + DMA

#define WS2812B_LEDS 35

typedef struct ws2812b_color {
	uint8_t red, green, blue;
} ws2812b_color;

void WS2812B_Init(SPI_HandleTypeDef * spi_handler);
void WS2812B_SetDiodeColor(int16_t diode_id, ws2812b_color color);
void WS2812B_SetDiodeRGB(int16_t diode_id, uint8_t R, uint8_t G, uint8_t B);
void WS2812B_Refresh();

#endif /* WS2812B_H_ */
