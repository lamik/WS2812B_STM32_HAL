/*
 * usb_parsing.c
 *
 *	The MIT License.
 *	Created on: 15.11.2018
 *		Author: Mateusz Salamon
 *		www.msalamon.pl
 *		mateusz@msalamon.pl
 */

#include "stm32f1xx_hal.h"
#include "usbd_cdc_if.h"
#include <stdlib.h>
#include <string.h>

#include "ws2812b.h"
#include "ws2812b_fx.h"
#include "usb_parsing.h"

uint8_t USBDataRX[50];			// Array for receive USB messages
uint8_t USBReceivedDataFlag;	// Received USB data flag
uint8_t USBDataTX[50]; 			// Array for transmission USB messages
uint8_t USBDataLength; 			// USB message length

void UnknownCommand(void)
{
	USBDataLength = sprintf((char*)USBDataTX, "Unknown command\n\r");
}

void ColorControl(void)
{
	char *buf;
	int16_t Seg;
	uint8_t Color[3];

	if((buf = strtok((char*)USBDataRX+1, ","))) // Segment number
	{
		Seg = atoi(buf);

		for(uint8_t i = 0; i < 3; i++)
		{
		  if((buf = strtok(NULL, ","))) // Speed
		  {
			  Color[i] = atoi(buf);
		  }
		  else
		  {
			  USBDataLength = sprintf((char*)USBDataTX, "Color command error\n\r");
		  	  return;
		  }
		}

		WS2812BFX_SetColorRGB(Seg, Color[0], Color[1], Color[2]);
		USBDataLength = sprintf((char*)USBDataTX, "ColorID:%d Value:%dR, %dG, %dB\n\r", Seg, Color[0], Color[1], Color[2]);
		return;
	}
	USBDataLength = sprintf((char*)USBDataTX, "Color command error\n\r");
}

void SpeedControl(void)
{
	char *buf;
	int16_t Seg;
	uint16_t Speed;

	if((buf = strtok((char*)USBDataRX+1, ","))) // Segment number
	{
	  Seg = atoi(buf);

	  if((buf = strtok(NULL, ","))) // Speed
	  {
		  if((Speed = atoi(buf)) > 0)
		  {
			  WS2812BFX_SetSpeed(Seg, Speed);
			  USBDataLength = sprintf((char*)USBDataTX, "Segment:%d Speed:%d\n\r", Seg, Speed);
			  return;
		  }
	  }
	}
	USBDataLength = sprintf((char*)USBDataTX, "Speed command error\n\r");
}

void ModeControl(void)
{
	char *buf;
	int16_t Seg;
	int16_t Mode;

	buf = strtok((char*)USBDataRX+1, ","); // Segment number
	Seg = atoi(buf);

	if((buf = strtok(NULL, ",")))
	{
	  if(buf[0] == 'S')
	  {
		  WS2812BFX_Start(Seg);
		  USBDataLength = sprintf((char*)USBDataTX, "Segment:%d Start\n\r", Seg);
		  return;
	  }
	  else if(buf[0] == 'T')
	  {
		  WS2812BFX_Stop(Seg);
		  USBDataLength = sprintf((char*)USBDataTX, "Segment:%d Stop\n\r", Seg);
		  return;
	  }
	  else  // Mode
	  {
		  if((Mode = atoi(buf)) > 0)
		  {
			  WS2812BFX_SetMode(Seg, Mode);

			  USBDataLength = sprintf((char*)USBDataTX, "Segment:%d Mode:%d\n\r", Seg, Mode);
			  return;
		  }
	  }
	}
	USBDataLength = sprintf((char*)USBDataTX, "mode command error\n\r");
}

void SegmentsControl(void)
{
	int16_t Seg;
	uint8_t mode_tmp;
	if(USBDataRX[1] == '-')
	{
		WS2812BFX_SegmentDecrease();
	}
	else if(USBDataRX[1] == '+')
	{
		WS2812BFX_SegmentIncrease();
		WS2812BFX_SetMode(WS2812BFX_GetSegmentsQuantity() - 1, rand()%MODE_COUNT);
		WS2812BFX_Start(WS2812BFX_GetSegmentsQuantity() - 1);
	}
	else if((Seg = atoi((char*)(USBDataRX+1))) > 0)
	{
		WS2812BFX_Init(Seg);
	}
	else
	{
		USBDataLength = sprintf((char*)USBDataTX, "Segment command error\n\r");
		return;
	}
	WS2812BFX_GetMode(WS2812BFX_GetSegmentsQuantity()-1, &mode_tmp);
	USBDataLength = sprintf((char*)USBDataTX, "Segments:%d Last mode:%d\n\r", WS2812BFX_GetSegmentsQuantity(), mode_tmp);
}

void SegmentRangeControl(void)
{
	char *buf;
	int16_t Seg;
	int16_t Start;
	int16_t End;

	buf = strtok((char*)USBDataRX+1, ","); // Segment number
	Seg = atoi(buf);

	if((buf = strtok(NULL, ",")))
	{
		if(buf[0] == 'R')	// Range (Stanrt and Stop)
		{
			if((buf = strtok(NULL, ","))) // Speed
			{
				if((Start = atoi(buf)) > 0)
				{
					  if((buf = strtok(NULL, ","))) // Speed
					  {
							if((End = atoi(buf)) > 0)
							{
								WS2812BFX_SetSegmentSize(Seg, Start, End);
								USBDataLength = sprintf((char*)USBDataTX, "Segment:%d Range:%d - %d\n\r", Seg, Start, End);
								return;
							}
							else
								USBDataLength = sprintf((char*)USBDataTX, "Segment range End error\n\r");
							return;
					  }
				}
				else
					USBDataLength = sprintf((char*)USBDataTX, "Segment range Start error\n\r");
				return;
			}
		}
		else if(buf[0] == 'S')	//Start
		{
			if(buf[1] == '+')
			{
				WS2812BFX_SegmentIncreaseStart(Seg);
				USBDataLength = sprintf((char*)USBDataTX, "Segment:%d Start increase\n\r", Seg);
				return;
			}
			if(buf[1] == '-')
			{
				WS2812BFX_SegmentDecreaseStart(Seg);
				USBDataLength = sprintf((char*)USBDataTX, "Segment:%d Start decrease\n\r", Seg);
				return;
			}

		}
		else if(buf[0] == 'E') //END
		{
			if(buf[1] == '+')
			{
				WS2812BFX_SegmentIncreaseEnd(Seg);
				USBDataLength = sprintf((char*)USBDataTX, "Segment:%d End increase\n\r", Seg);
				return;
			}
			if(buf[1] == '-')
			{
				WS2812BFX_SegmentDecreaseEnd(Seg);
				USBDataLength = sprintf((char*)USBDataTX, "Segment:%d End decrease\n\r", Seg);
				return;
			}
		}
	}
	USBDataLength = sprintf((char*)USBDataTX, "Segment range command error\n\r");
}

void PrintHelp(void)
{

	USBDataLength = sprintf((char*)USBDataTX, "\033[2J\033[0;0H");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "==============HELP=============\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "Change Segments quantity:\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Sx'  Set x segments(1 - LEDs/2)\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'S+' add one segment\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'S-' remove one segment\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "Change Segments length:\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Rx,S+' Increase start point x ");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "segment\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Rx,S-' Decrease start point x ");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "segment\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Rx,E+' Increase end point x ");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "segment\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Rx,E-' Decrease end point x ");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "segment\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Rx,R,y,z' Set start(y) and stop(z) po");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "int x segment\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "Change segment's mode:\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Mx,S' Start segment x\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Mx,T' Stop segment x\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Mx,y' Set y mode for x segment\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "Change segment's speed:\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Vx,y' Set y speed for x segment\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "Set color:\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "  'Cx,r,g,b' x - ColorID, rgb values\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
	USBDataLength = sprintf((char*)USBDataTX, "===============================\n\r");
	while(USBD_BUSY == CDC_Transmit_FS(USBDataTX, USBDataLength));
}

void USB_Parsing(void)
{
	if(USBReceivedDataFlag == 1)
	{
	  USBReceivedDataFlag = 0;

	  switch (USBDataRX[0])
	  {
		case 'C':
			ColorControl();
			break;

		case 'V':
			SpeedControl();
			break;

		case 'M':
			ModeControl();
			break;

		case 'S':
			SegmentsControl();
			break;

		case 'R':
			SegmentRangeControl();
			break;

		case 'H':
			PrintHelp();
			break;
		default:
			UnknownCommand();
			break;
	  }

	  CDC_Transmit_FS(USBDataTX, USBDataLength); // Send confirmation message
	}
}
