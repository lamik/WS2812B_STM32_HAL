/*
 * ws2812b.c
 *
 *	The MIT License.
 *	Created on: 14.07.2017
 *		Author: Mateusz Salamon
 *		www.msalamon.pl
 *		mateusz@msalamon.pl
 */

#include "stm32f1xx_hal.h"
#include "spi.h"
#include "dma.h"
#include "gpio.h"
#include "math.h"

#include "ws2812b.h"

#define zero 0b00000011
#define one 0b00011111

SPI_HandleTypeDef *hspi_ws2812b;
ws2812b_color ws2812b_array[WS2812B_LEDS];

static uint8_t buffer[48];
static uint16_t CurrentLed;
static uint8_t ResetSignal;

void WS2812B_Init(SPI_HandleTypeDef * spi_handler)
{
	hspi_ws2812b = spi_handler;
}

void WS2812B_SetDiodeColor(int16_t diode_id, ws2812b_color color)
{
	if(diode_id >= WS2812B_LEDS || diode_id < 0) return;
	ws2812b_array[diode_id] = color;
}

void WS2812B_SetDiodeRGB(int16_t diode_id, uint8_t R, uint8_t G, uint8_t B)
{
	if(diode_id >= WS2812B_LEDS || diode_id < 0) return;
	ws2812b_array[diode_id].red = R;
	ws2812b_array[diode_id].green = G;
	ws2812b_array[diode_id].blue = B;
}

//
//	Set diode with HSV model
//
//	Hue 0-359
//	Saturation 0-255
//	Birghtness(Value) 0-255
//
void WS2812B_SetDiodeHSV(int16_t diode_id, uint16_t Hue, uint8_t Saturation, uint8_t Brightness)
{
	if(diode_id >= WS2812B_LEDS || diode_id < 0) return;
	uint16_t Sector, Fracts, p, q, t;

	if(Saturation == 0)
	{
		ws2812b_array[diode_id].red = Brightness;
		ws2812b_array[diode_id].green = Brightness;
		ws2812b_array[diode_id]. blue = Brightness;
	}
	else
	{
		if(Hue >= 360) Hue = 359;

		Sector = Hue / 60; // Sector 0 to 5
		Fracts = Hue % 60;
		p = (Brightness * (255 - Saturation)) / 256;
		q = (Brightness * (255 - (Saturation * Fracts)/360)) / 256;
		t = (Brightness * (255 - (Saturation * (360 - Fracts))/360)) / 256;


		switch(Sector)
		{
		case 0:
			ws2812b_array[diode_id].red = Brightness;
			ws2812b_array[diode_id].green = (uint8_t)t;
			ws2812b_array[diode_id]. blue = (uint8_t)p;
			break;
		case 1:
			ws2812b_array[diode_id].red = (uint8_t)q;
			ws2812b_array[diode_id].green = Brightness;
			ws2812b_array[diode_id]. blue = (uint8_t)p;
			break;
		case 2:
			ws2812b_array[diode_id].red = (uint8_t)p;
			ws2812b_array[diode_id].green = Brightness;
			ws2812b_array[diode_id]. blue = (uint8_t)t;
			break;
		case 3:
			ws2812b_array[diode_id].red = (uint8_t)p;
			ws2812b_array[diode_id].green = (uint8_t)q;
			ws2812b_array[diode_id]. blue = Brightness;
			break;
		case 4:
			ws2812b_array[diode_id].red = (uint8_t)t;
			ws2812b_array[diode_id].green = (uint8_t)p;
			ws2812b_array[diode_id]. blue = Brightness;
			break;
		default:		// case 5:
			ws2812b_array[diode_id].red = Brightness;
			ws2812b_array[diode_id].green = (uint8_t)p;
			ws2812b_array[diode_id]. blue = (uint8_t)q;
			break;
		}
	}
}

void WS2812B_Refresh()
{
	CurrentLed = 0;
	ResetSignal = 0;

	for(uint8_t i = 0; i < 48; i++)
		buffer[i] = 0x00;
	HAL_SPI_Transmit_DMA(hspi_ws2812b, buffer, 48); // Additional 3 for reset signal
	while(HAL_DMA_STATE_READY != HAL_DMA_GetState(hspi_ws2812b->hdmatx));

}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == hspi_ws2812b)
	{
		if(!ResetSignal)
		{
			for(uint8_t k = 0; k < 24; k++) // To 72 impulses of reset
			{
				buffer[k] = 0x00;
			}
			ResetSignal = 1; // End reset signal
		}
		else // LEDs Odd 1,3,5,7...
		{
			if(CurrentLed > WS2812B_LEDS)
			{
				HAL_SPI_DMAStop(hspi_ws2812b);
			}
			else
			{
				uint8_t j = 0;
				//GREEN
				for(int8_t k=7; k>=0; k--)
				{
					if((ws2812b_array[CurrentLed].green & (1<<k)) == 0)
						buffer[j] = zero;
					else
						buffer[j] = one;
					j++;
				}

				//RED
				for(int8_t k=7; k>=0; k--)
				{
					if((ws2812b_array[CurrentLed].red & (1<<k)) == 0)
						buffer[j] = zero;
					else
						buffer[j] = one;
					j++;
				}

				//BLUE
				for(int8_t k=7; k>=0; k--)
				{
					if((ws2812b_array[CurrentLed].blue & (1<<k)) == 0)
						buffer[j] = zero;
					else
						buffer[j] = one;
					j++;
				}
				CurrentLed++;
			}
		}
	}
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == hspi_ws2812b)
	{
		if(CurrentLed > WS2812B_LEDS)
		{
			HAL_SPI_DMAStop(hspi_ws2812b);
		}
		else
		{
			// Even LEDs 0,2,0
			uint8_t j = 24;
			//GREEN
			for(int8_t k=7; k>=0; k--)
			{
				if((ws2812b_array[CurrentLed].green & (1<<k)) == 0)
					buffer[j] = zero;
				else
					buffer[j] = one;
				j++;
			}

			//RED
			for(int8_t k=7; k>=0; k--)
			{
				if((ws2812b_array[CurrentLed].red & (1<<k)) == 0)
					buffer[j] = zero;
				else
					buffer[j] = one;
				j++;
			}

			//BLUE
			for(int8_t k=7; k>=0; k--)
			{
				if((ws2812b_array[CurrentLed].blue & (1<<k)) == 0)
					buffer[j] = zero;
				else
					buffer[j] = one;
				j++;
			}
			CurrentLed++;
		}
	}

}
