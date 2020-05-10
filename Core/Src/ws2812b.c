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

#define zero 0b11000000
#define one 0b11111000

SPI_HandleTypeDef *hspi_ws2812b;
ws2812b_color ws2812b_array[WS2812B_LEDS];

static uint8_t buffer[48];
static uint16_t CurrentLed;
static uint8_t ResetSignal;

void WS2812B_Init(SPI_HandleTypeDef * spi_handler)
{
	hspi_ws2812b = spi_handler;
}

void WS2812B_SetDiodeColor(int16_t diode_id, uint32_t color)
{
	if(diode_id >= WS2812B_LEDS || diode_id < 0) return;
	ws2812b_array[diode_id].red = ((color>>16)&0x000000FF);
	ws2812b_array[diode_id].green = ((color>>8)&0x000000FF);
	ws2812b_array[diode_id].blue = (color&0x000000FF);
}

void WS2812B_SetDiodeColorStruct(int16_t diode_id, ws2812b_color color)
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

uint32_t WS2812B_GetColor(int16_t diode_id)
{
	uint32_t color = 0;
	color |= ((ws2812b_array[diode_id].red&0xFF)<<16);
	color |= ((ws2812b_array[diode_id].green&0xFF)<<8);
	color |= (ws2812b_array[diode_id].blue&0xFF);
	return color;
}

uint8_t* WS2812B_GetPixels(void)
{
	return (uint8_t*)ws2812b_array;
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
		q = (Brightness * (255 - (Saturation * Fracts)/60)) / 256;
		t = (Brightness * (255 - (Saturation * (59 - Fracts))/60)) / 256;


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

static const uint8_t _sineTable[256] = {
  128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
  176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
  218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
  245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
  255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
  245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
  218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
  176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
  128,124,121,118,115,112,109,106,103,100, 97, 93, 90, 88, 85, 82,
   79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
   37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11,
   10,  9,  7,  6,  5,  5,  4,  3,  2,  2,  1,  1,  1,  0,  0,  0,
    0,  0,  0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  5,  6,  7,  9,
   10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
   37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
   79, 82, 85, 88, 90, 93, 97,100,103,106,109,112,115,118,121,124};

static const uint8_t _gammaTable[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
    3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  7,
    7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,
   13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
   20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29,
   30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42,
   42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
   58, 59, 60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 75,
   76, 77, 78, 80, 81, 82, 84, 85, 86, 88, 89, 90, 92, 93, 94, 96,
   97, 99,100,102,103,105,106,108,109,111,112,114,115,117,119,120,
  122,124,125,127,129,130,132,134,136,137,139,141,143,145,146,148,
  150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,
  182,184,186,188,191,193,195,197,199,202,204,206,209,211,213,215,
  218,220,223,225,227,230,232,235,237,240,242,245,247,250,252,255};

uint8_t sine8(uint8_t x)
{
	return _sineTable[x];
}

uint8_t gamma8(uint8_t x)
{
	return _gammaTable[x];
}
