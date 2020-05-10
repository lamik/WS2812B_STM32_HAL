/*
 * ws2812b_fx.c
 *
 * 	Library based on Harm Aldick's Arduino library
 *	https://github.com/kitesurfer1404/WS2812FX
 *
 *	The MIT License.
 *	Created on: 20.10.2018
 *		Author: Mateusz Salamon
 *		www.msalamon.pl
 *		mateusz@msalamon.pl
 */
#include "stm32f1xx_hal.h"
#include <stdlib.h>

#include "ws2812b.h"
#include "ws2812b_fx.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)<(b))?(b):(a))

#define SEGMENT_LENGTH   (Ws28b12b_Segments[mActualSegment].IdStop - Ws28b12b_Segments[mActualSegment].IdStart + 1)
#define IS_REVERSE		Ws28b12b_Segments[mActualSegment].Reverse

uint8_t 	mRunning;
uint8_t 	mTriggered;
uint8_t		mActualSegment;

uint16_t 	mSegments;

uint32_t		mColor[NUM_COLORS];
ws2812b_color	mColor_w[NUM_COLORS];

typedef struct ws2812bfx_s
{
	volatile uint32_t	ModeDelay;	// Segment SW timer counter

	uint16_t	IdStart;			// Start segment point
	uint16_t	IdStop;				// End segment point

	uint8_t 	Running : 1;		// Is sector running
	uint8_t		ActualMode; 		// Sector mode setting
	uint8_t		Reverse : 1;		// Is reverted mode
	uint32_t	CounterModeCall;	// Numbers of calls
	uint32_t	CounterModeStep;	// Call step

	uint16_t		Speed;			// Segment speed

	uint32_t		ModeColor[NUM_COLORS];		// Mode color 32 bit representation
	ws2812b_color	ModeColor_w[NUM_COLORS]; 	// Mode color struct representation

	uint8_t 	AuxParam;			// Computing variable
	uint16_t 	AuxParam16b;		// Computing variable
	uint8_t 	Cycle : 1;			// Cycle variable

	void 	(*mModeCallback)(void); // Sector mode callback
} ws2812bfx_s;

ws2812bfx_s *Ws28b12b_Segments = NULL;

void (*mModeCallback)(void);


/*
 *
 *  MODES
 *
 * */
void
strip_off(void),
mode_static(void),
mode_white_to_color(void),
mode_black_to_color(void),
mode_blink(void),
mode_blink_rainbow(void),
mode_strobe(void),
mode_strobe_rainbow(void),
mode_breath(void),
mode_color_wipe(void),
mode_color_wipe_inv(void),
mode_color_wipe_rev(void),
mode_color_wipe_rev_inv(void),
mode_color_wipe_random(void),
mode_color_sweep_random(void),
mode_random_color(void),
mode_single_dynamic(void),
mode_multi_dynamic(void),
mode_rainbow(void),
mode_rainbow_cycle(void),
mode_fade(void),
mode_scan(void),
mode_dual_scan(void),
mode_theater_chase(void),
mode_theater_chase_rainbow(void),
mode_running_lights(void),
mode_twinkle(void),
mode_twinkle_random(void),
mode_twinkle_fade(void),
mode_twinkle_fade_random(void),
mode_sparkle(void),
mode_flash_sparkle(void),
mode_hyper_sparkle(void),
mode_multi_strobe(void),
mode_chase_white(void),
mode_chase_color(void),
mode_chase_random(void),
mode_chase_rainbow(void),
mode_chase_flash(void),
mode_chase_flash_random(void),
mode_chase_rainbow_white(void),
mode_chase_blackout(void),
mode_chase_blackout_rainbow(void),
mode_running_color(void),
mode_running_red_blue(void),
mode_running_random(void),
mode_larson_scanner(void),
mode_comet(void),
mode_fireworks(void),
mode_fireworks_random(void),
mode_merry_christmas(void),
mode_fire_flicker(void),
mode_fire_flicker_soft(void),
mode_fire_flicker_intense(void),
mode_circus_combustus(void),
mode_halloween(void),
mode_bicolor_chase(void),
mode_tricolor_chase(void),
mode_icu(void)
;

void (*mMode[MODE_COUNT])(void) =
{
	mode_static,
	mode_white_to_color,
	mode_black_to_color,
	mode_blink,
	mode_blink_rainbow,
	mode_strobe,
	mode_strobe_rainbow,
	mode_breath,
	mode_color_wipe,
	mode_color_wipe_inv,
	mode_color_wipe_rev,
	mode_color_wipe_rev_inv,
	mode_color_wipe_random,
	mode_color_sweep_random,
	mode_random_color,
    mode_single_dynamic,
    mode_multi_dynamic,
    mode_rainbow,
    mode_rainbow_cycle,
    mode_fade,
    mode_scan,
    mode_dual_scan,
    mode_theater_chase,
    mode_theater_chase_rainbow,
    mode_running_lights,
    mode_twinkle,
    mode_twinkle_random,
	mode_twinkle_fade,
	mode_twinkle_fade_random,
    mode_sparkle,
    mode_flash_sparkle,
    mode_hyper_sparkle,
	mode_multi_strobe,
    mode_chase_white,
    mode_chase_color,
    mode_chase_random,
    mode_chase_rainbow,
    mode_chase_flash,
    mode_chase_flash_random,
    mode_chase_rainbow_white,
    mode_chase_blackout,
    mode_chase_blackout_rainbow,
    mode_running_color,
    mode_running_red_blue,
    mode_running_random,
    mode_larson_scanner,
    mode_comet,
    mode_fireworks,
    mode_fireworks_random,
    mode_merry_christmas,
    mode_fire_flicker,
    mode_fire_flicker_soft,
    mode_fire_flicker_intense,
    mode_circus_combustus,
    mode_halloween,
    mode_bicolor_chase,
    mode_tricolor_chase,
    mode_icu
};

FX_STATUS WS2812BFX_Init(uint16_t Segments)
{
	if(Segments == 0) return FX_ERROR;
	if(Segments > (WS2812B_LEDS / 2))
	{
		if(Segments > WS2812B_LEDS)
		{
			return FX_ERROR;
		}
	}

	uint16_t div = 0;
	ws2812bfx_s *SegmentsTmp = NULL;

	SegmentsTmp = calloc(Segments, sizeof(ws2812bfx_s));	// Assign the space for new segments

	if(SegmentsTmp == NULL) return FX_ERROR;	// If assigning failed

	if(Ws28b12b_Segments == NULL)
	{
		mSegments = Segments;

		for(uint16_t i = 0; i < mSegments; i++)
		{
			SegmentsTmp[i].Speed = DEFAULT_SPEED;
			SegmentsTmp[i].Running = DEFAULT_MODE;

			SegmentsTmp[i].IdStart = div;
			div += ((WS2812B_LEDS + 1) / Segments) - 1;
			SegmentsTmp[i].IdStop = div;
			if(SegmentsTmp[i].IdStop >= WS2812B_LEDS) SegmentsTmp[i].IdStop = WS2812B_LEDS - 1;
			div++;
		}
	}
	else	// Ws28b12b_Segments was before initialized
	{
		for(uint16_t i = 0; i < (Segments>mSegments?mSegments:Segments); i++)
		{
			SegmentsTmp[i].ModeDelay = Ws28b12b_Segments[i].ModeDelay;

			SegmentsTmp[i].IdStart = div;
			div += ((WS2812B_LEDS + 1) / Segments) - 1;
			SegmentsTmp[i].IdStop = div;
			if(SegmentsTmp[i].IdStop >= WS2812B_LEDS) Ws28b12b_Segments[i].IdStop = WS2812B_LEDS - 1;
			div++;

			SegmentsTmp[i].Running = Ws28b12b_Segments[i].Running;
			SegmentsTmp[i].ActualMode = Ws28b12b_Segments[i].ActualMode;
			SegmentsTmp[i].Reverse = Ws28b12b_Segments[i].Reverse;
			SegmentsTmp[i].CounterModeCall = Ws28b12b_Segments[i].CounterModeCall;
			SegmentsTmp[i].CounterModeStep = Ws28b12b_Segments[i].CounterModeStep;
			SegmentsTmp[i].Speed = Ws28b12b_Segments[i].Speed;
			for(uint8_t j = 0; j < NUM_COLORS; j++)
			{
				SegmentsTmp[i].ModeColor[j] = Ws28b12b_Segments[i].ModeColor[j];
				SegmentsTmp[i].ModeColor_w[j] = Ws28b12b_Segments[i].ModeColor_w[j];
			}
			SegmentsTmp[i].AuxParam = Ws28b12b_Segments[i].AuxParam;
			SegmentsTmp[i].AuxParam16b = Ws28b12b_Segments[i].AuxParam16b;
			SegmentsTmp[i].Cycle = Ws28b12b_Segments[i].Cycle;
			SegmentsTmp[i].mModeCallback = Ws28b12b_Segments[i].mModeCallback;
		}

		if(Segments > mSegments) // Add new Segment
		{
			SegmentsTmp[Segments - 1].Speed = DEFAULT_SPEED;
			SegmentsTmp[Segments - 1].ActualMode = DEFAULT_MODE;
			SegmentsTmp[Segments - 1].Running = 0; // Sany new segment is stopped by default

			SegmentsTmp[Segments - 1].IdStart = div;
			div += ((WS2812B_LEDS + 1) / Segments) - 1;
			SegmentsTmp[Segments - 1].IdStop = WS2812B_LEDS - 1;
		}

		mSegments = Segments;
	}

	free(Ws28b12b_Segments);	// Free previous array if reinit
	Ws28b12b_Segments = SegmentsTmp;
	return FX_OK;
}

uint8_t WS2812BFX_GetSegmentsQuantity(void)
{
	return mSegments;
}

FX_STATUS WS2812BFX_SegmentIncrease(void)
{
	if(mSegments < (WS2812B_LEDS - 1))
	{
	 WS2812BFX_Init(mSegments + 1);
	 return FX_OK;
	}
	return FX_ERROR;
}

FX_STATUS WS2812BFX_SegmentDecrease(void)
{
	if(mSegments > 1)
	{
		WS2812BFX_Init(mSegments - 1);
	 return FX_OK;
	}
	return FX_ERROR;
}

void WS2812BFX_SysTickCallback(void)
{
	for(uint16_t i = 0; i < mSegments; i++)
		if(Ws28b12b_Segments[i].ModeDelay > 0) Ws28b12b_Segments[i].ModeDelay--;
}

void WS2812BFX_Callback()
 {
	static uint8_t trig = 0;;
  if(mRunning || mTriggered)
  {
	  for(uint16_t i = 0; i < mSegments; i++)
	  {
		  if(Ws28b12b_Segments[i].ModeDelay == 0)
		  {
			  if(Ws28b12b_Segments[i].Running)
			  {
				  mActualSegment = i;
				  Ws28b12b_Segments[i].mModeCallback();
				  Ws28b12b_Segments[i].CounterModeCall++;
				  trig = 1;
			  }
		  }
	  }
	  if(trig)
	  {
		  WS2812B_Refresh();
		  trig = 0;
	  }
  }
}

FX_STATUS WS2812BFX_SetMode(uint16_t Segment, fx_mode Mode)
{
	if(Segment >= mSegments) return FX_ERROR;
	Ws28b12b_Segments[Segment].CounterModeCall = 0;
	Ws28b12b_Segments[Segment].CounterModeStep = 0;
	Ws28b12b_Segments[Segment].ActualMode = Mode;
	Ws28b12b_Segments[Segment].mModeCallback = mMode[Mode];
	for(uint8_t i = 0; i < NUM_COLORS; i++)
	{
		Ws28b12b_Segments[Segment].ModeColor[i] = mColor[i];
		Ws28b12b_Segments[Segment].ModeColor_w[i].red = mColor_w[i].red;
		Ws28b12b_Segments[Segment].ModeColor_w[i].green = mColor_w[i].green;
		Ws28b12b_Segments[Segment].ModeColor_w[i].blue = mColor_w[i].blue;
	}
	return FX_OK;
}

FX_STATUS WS2812BFX_GetMode(uint16_t Segment, fx_mode *Mode)
{
	if(Segment >= mSegments) return FX_ERROR;
	*Mode = Ws28b12b_Segments[Segment].ActualMode;
	return FX_OK;
}

FX_STATUS WS2812BFX_NextMode(uint16_t Segment)
{
	if(Segment >= mSegments) return FX_ERROR;
	Ws28b12b_Segments[Segment].CounterModeCall = 0;
	Ws28b12b_Segments[Segment].CounterModeStep = 0;
	Ws28b12b_Segments[Segment].ActualMode++;
	if(Ws28b12b_Segments[Segment].ActualMode >= MODE_COUNT) Ws28b12b_Segments[mActualSegment].ActualMode = 0;
	Ws28b12b_Segments[Segment].mModeCallback = mMode[Ws28b12b_Segments[mActualSegment].ActualMode];
	return FX_OK;
}

FX_STATUS WS2812BFX_PrevMode(uint16_t Segment)
{
	if(Segment >= mSegments) return FX_ERROR;
	Ws28b12b_Segments[Segment].CounterModeCall = 0;
	Ws28b12b_Segments[Segment].CounterModeStep = 0;
	Ws28b12b_Segments[Segment].ActualMode--;
	if(Ws28b12b_Segments[Segment].ActualMode == 0) Ws28b12b_Segments[mActualSegment].ActualMode = MODE_COUNT;
	Ws28b12b_Segments[Segment].mModeCallback = mMode[Ws28b12b_Segments[mActualSegment].ActualMode];
	return FX_OK;
}

FX_STATUS WS2812BFX_SetReverse(uint16_t Segment, uint8_t Reverse)
{
	if(Segment >= mSegments) return FX_ERROR;
	WS2812BFX_SetAll(Segment, BLACK); // Set all 'old' segment black

	if(Reverse > 1) Reverse = 1;

	Ws28b12b_Segments[Segment].Reverse = Reverse;
	return FX_OK;
}

FX_STATUS WS2812BFX_GetReverse(uint16_t Segment, uint8_t *Reverse)
{
	if(Segment >= mSegments) return FX_ERROR;
	*Reverse = Ws28b12b_Segments[Segment].Reverse;
	return FX_OK;
}

FX_STATUS WS2812BFX_SegmentIncreaseStart(uint16_t Segment)
{
	if(Segment >= mSegments) return FX_ERROR;
	WS2812BFX_SetAll(Segment, BLACK); // Set all 'old' segment black

	if((Ws28b12b_Segments[Segment].IdStop - Ws28b12b_Segments[Segment].IdStart) > 0)
	{
		Ws28b12b_Segments[Segment].IdStart++;
	}
	return FX_OK;
}

FX_STATUS WS2812BFX_SegmentDecreaseStart(uint16_t Segment)
{
	if(Segment >= mSegments) return FX_ERROR;
	WS2812BFX_SetAll(Segment, BLACK); // Set all 'old' segment black

	if(Segment > 0)
	{
		if(Ws28b12b_Segments[Segment-1].IdStop < (Ws28b12b_Segments[Segment].IdStart - 1))
		{
			Ws28b12b_Segments[Segment].IdStart--;
		}
	}
	else // Segment == 0
	{
		if(0 < Ws28b12b_Segments[Segment].IdStart)
		{
			Ws28b12b_Segments[Segment].IdStart--;
		}
	}
	return FX_OK;
}

FX_STATUS WS2812BFX_SegmentIncreaseEnd(uint16_t Segment)
{
	if(Segment >= mSegments) return FX_ERROR;
	WS2812BFX_SetAll(Segment, BLACK); // Set all 'old' segment black

	if(Segment < (mSegments - 1))
	{
		if(Ws28b12b_Segments[Segment].IdStop < (Ws28b12b_Segments[Segment+1].IdStart - 1))
		{
			Ws28b12b_Segments[Segment].IdStop++;
		}

	}
	else // last Segment
	{
		if(Ws28b12b_Segments[Segment].IdStop < (WS2812B_LEDS - 1))
		{
			Ws28b12b_Segments[Segment].IdStop++;

		}
	}
	return FX_OK;
}

FX_STATUS WS2812BFX_SegmentDecreaseEnd(uint16_t Segment)
{
	if(Segment >= mSegments) return FX_ERROR;
	WS2812BFX_SetAll(Segment, BLACK); // Set all 'old' segment black

	if((Ws28b12b_Segments[Segment].IdStop - Ws28b12b_Segments[Segment].IdStart) > 0)
	{
		Ws28b12b_Segments[Segment].IdStop--;
	}
	return FX_OK;
}

FX_STATUS WS2812BFX_SetSegmentSize(uint16_t Segment, uint16_t Start, uint16_t Stop)
{
	if(Segment >= mSegments) return FX_ERROR;
	if(Start >= (WS2812B_LEDS - 1)) return FX_ERROR;
	if(Stop >= (WS2812B_LEDS - 1)) return FX_ERROR;
	if(Start > Stop) return FX_ERROR;

	WS2812BFX_SetAll(Segment, BLACK); // Set all 'old' segment black

	Ws28b12b_Segments[Segment].IdStart = Start;
	Ws28b12b_Segments[Segment].IdStop = Stop;
	return FX_OK;
}

FX_STATUS WS2812BFX_GetSegmentSize(uint16_t Segment, uint16_t *Start, uint16_t *Stop)
{
	if(Segment >= mSegments) return FX_ERROR;
	*Start = Ws28b12b_Segments[Segment].IdStart;
	*Stop = Ws28b12b_Segments[Segment].IdStop;
	return FX_OK;
}

FX_STATUS WS2812BFX_Start(uint16_t Segment)
{
	if(Segment >= mSegments) return FX_ERROR;
	Ws28b12b_Segments[Segment].CounterModeCall = 0;
	Ws28b12b_Segments[Segment].CounterModeStep = 0;
	Ws28b12b_Segments[Segment].ModeDelay = 0;
	Ws28b12b_Segments[Segment].Running = 1;
	mRunning = 1;
	return FX_OK;
}

uint8_t WS2812BFX_IsAnyRunning(void)
{
	for(uint8_t i = 0; i < mSegments; i++)
	{
		if(Ws28b12b_Segments[i].Running != 0)
			return 1;
	}

	return 0;
}

FX_STATUS WS2812BFX_Stop(uint16_t Segment)
{
	if(Segment >= mSegments) return FX_ERROR;
	Ws28b12b_Segments[Segment].Running = 0;
	if(!WS2812BFX_IsAnyRunning())
		mRunning = 0;
	return FX_OK;
//	  strip_off();
}

FX_STATUS WS2812BFX_IsRunning(uint16_t Segment, uint8_t *Running)
{
	if(Segment >= mSegments) return FX_ERROR;
	*Running = Ws28b12b_Segments[Segment].Running;
	return FX_OK;
}

void WS2812BFX_SetColorStruct(uint8_t id, ws2812b_color c)
{
	mColor[id] = ((c.red<<16)|(c.green<<8)|c.blue);
	mColor_w[id].red = c.red;
	mColor_w[id].green = c.green;
	mColor_w[id].blue = c.blue;
}

void WS2812BFX_SetColorRGB(uint8_t id, uint8_t r, uint8_t g, uint8_t b)
{
	mColor[id] = ((r<<16)|(g<<8)|b);
	mColor_w[id].red = r;
	mColor_w[id].green = g;
	mColor_w[id].blue = b;

}

FX_STATUS WS2812BFX_GetColorRGB(uint8_t id, uint8_t *r, uint8_t *g, uint8_t *b)
{
	if(id >= NUM_COLORS) return FX_ERROR;
	*r = mColor_w[id].red;
	*g = mColor_w[id].green;
	*b = mColor_w[id].blue;
	return FX_OK;
}

void WS2812BFX_RGBtoHSV(uint8_t r, uint8_t g, uint8_t b, uint16_t *h, uint8_t *s, uint8_t *v)
{
    uint16_t min, max, delta;
    int16_t h_tmp = *h;

    min = r < g ? r : g;
    min = min  < b ? min : b;

    max = r > g ? r : g;
    max = max  > b ? max : b;

    *v = max;	// v
    delta = max - min;
    if (delta < 1)
    {
        *s = 0;
        *h = 0; // undefined, maybe nan?
        return;
    }
    if( max > 0 )
    { // NOTE: if Max is == 0, this divide would cause a crash
        *s = (((delta * 100) / max) * 255) / 100; // s
    }
    else
    {
        // if max is 0, then r = g = b = 0
        // s = 0, h is undefined
        *s = 0;
        *h = 0;                            // its now undefined
        return;
    }

    if( r == max )
    {
    	h_tmp = (( g - b )*100) / delta;        // between yellow & magenta
    											// *100 to avoid fracts
    }
    else
    {
		if( g == max )
		{
			h_tmp = 720 + (( b - r )*100) / delta;  // between cyan & yellow
		}
		else
		{
			h_tmp = 1440 + (( r - g )*100) / delta;  // between magenta & cyan
		}
    }
    h_tmp *= 60; // Degrees
    h_tmp /= 100; // Back from fracts

    if( h_tmp < 0.0 )
    	h_tmp += 360;

    *h = h_tmp;
}

void WS2812BFX_HSVtoRGB(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint16_t Sector, Fracts, p, q, t;

	if(s == 0)
	{

		*r = v;
		*g = v;
		*b = v;
	}
	else
	{
		if(h >= 360) h = 359;

		Sector = h / 60; // Sector 0 to 5
		Fracts = h % 60;
		p = (v * (255 - s)) / 256;
		q = (v * (255 - (s * Fracts)/60)) / 256;
		t = (v * (255 - (s * (59 - Fracts))/60)) / 256;


		switch(Sector)
		{
		case 0:
			*r = v;
			*g = (uint8_t)t;
			*b = (uint8_t)p;
			break;
		case 1:
			*r = (uint8_t)q;
			*g = v;
			*b = (uint8_t)p;
			break;
		case 2:
			*r = (uint8_t)p;
			*g = v;
			*b = (uint8_t)t;
			break;
		case 3:
			*r = (uint8_t)p;
			*g = (uint8_t)q;
			*b = v;
			break;
		case 4:
			*r = (uint8_t)t;
			*g = (uint8_t)p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = (uint8_t)p;
			*b = (uint8_t)q;
			break;
		}
	}
}

//
//	Set color with HSV model
//
//	Hue 		0-359
//	Saturation 	0-255
//	Value 		0-255
//
void WS2812BFX_SetColorHSV(uint8_t id, uint16_t h, uint8_t s, uint8_t v)
{
	WS2812BFX_HSVtoRGB(h, s, v, &mColor_w[id].red, &mColor_w[id].green, &mColor_w[id].blue);
	mColor[id] = ((mColor_w[id].red<<16)|(mColor_w[id].green<<8)|mColor_w[id]. blue);
}

void WS2812BFX_SetColor(uint8_t id, uint32_t c)
{
	mColor[id] = c;
	mColor_w[id].red = ((c>>16)&0x000000FF);
	mColor_w[id].green = ((c>>8)&0x000000FF);
	mColor_w[id].blue = (c&0x000000FF);
}

FX_STATUS WS2812BFX_SetAll(uint16_t Segment, uint32_t c)
{
	if(Segment >= mSegments) return FX_ERROR;
	for(uint16_t i = 0; i < (Ws28b12b_Segments[Segment].IdStop - Ws28b12b_Segments[Segment].IdStart + 1); i++)
	{
		WS2812B_SetDiodeRGB(Ws28b12b_Segments[Segment].IdStart + i, ((c>>16)&0xFF), ((c>>8)&0xFF), (c&0xFF));
	}
	return FX_OK;
}

FX_STATUS WS2812BFX_SetAllRGB(uint16_t Segment, uint8_t r, uint8_t g, uint8_t b)
{
	if(Segment >= mSegments) return FX_ERROR;
	for(uint16_t i = 0; i < SEGMENT_LENGTH; i++)
	{
		WS2812B_SetDiodeRGB(Ws28b12b_Segments[Segment].IdStart + i, r, g, b);
	}
	return FX_OK;
}

FX_STATUS WS2812BFX_SetSpeed(uint16_t Segment, uint16_t Speed)
{
	if(Segment >= mSegments) return FX_ERROR;
	if(Speed < SPEED_MIN) Speed = SPEED_MIN;
	if(Speed > SPEED_MAX) Speed = SPEED_MAX;

	Ws28b12b_Segments[Segment].Speed = Speed;
	return FX_OK;
}

FX_STATUS WS2812BFX_GetSpeed(uint16_t Segment, uint16_t *Speed)
{
	if(Segment >= mSegments) return FX_ERROR;
	*Speed = Ws28b12b_Segments[Segment].Speed;
	return FX_OK;
}

FX_STATUS WS2812BFX_IncreaseSpeed(uint16_t Segment, uint16_t Speed)
{
	return WS2812BFX_SetSpeed(Segment, Ws28b12b_Segments[mActualSegment].Speed + Speed);
}

FX_STATUS WS2812BFX_DecreaseSpeed(uint16_t Segment, uint16_t Speed)
{
	return WS2812BFX_SetSpeed(Segment, Ws28b12b_Segments[mActualSegment].Speed - Speed);
}

//
//
// COLOR EFECTS FUNCTIONS
//
//

/*
 * Turns everything off.
 */
void strip_off()
{
	for(uint16_t i = 0; i < WS2812B_LEDS; i++)
	{
		WS2812B_SetDiodeRGB(i, 0, 0, 0);
	}
	WS2812B_Refresh();
}

/*
 * Put a value 0 to 255 in to get a color value.
 * The colours are a transition r -> g -> b -> back to r
 * Inspired by the Adafruit examples.
 */
uint32_t color_wheel(uint8_t pos) {
  pos = 255 - pos;
  if(pos < 85) {
    return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
  } else if(pos < 170) {
    pos -= 85;
    return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
  } else {
    pos -= 170;
    return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
  }
}

/*
 * Returns a new, random wheel index with a minimum distance of 42 from pos.
 */
uint8_t get_random_wheel_index(uint8_t pos) {
  uint8_t r = 0;
  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t d = 0;

  while(d < 42) {
    r = rand()%256;
    x = abs(pos - r);
    y = 255 - x;
    d = MIN(x, y);
  }

  return r;
}

/*
 * fade out function
 */
void fade_out() {
  static const uint8_t rateMapH[] = {0, 1, 1, 1, 2, 3, 4, 6};
  static const uint8_t rateMapL[] = {0, 2, 3, 8, 8, 8, 8, 8};

  uint8_t rate  = FADE_RATE;
  uint8_t rateH = rateMapH[rate];
  uint8_t rateL = rateMapL[rate];

  uint32_t color = Ws28b12b_Segments[mActualSegment].ModeColor[1]; // target color

  int r2 = (color >> 16) & 0xff;
  int g2 = (color >>  8) & 0xff;
  int b2 =  color        & 0xff;

  for(uint16_t i=Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++) {

    color = WS2812B_GetColor(i);
    if(rate == 0) { // old fade-to-black algorithm
    	WS2812B_SetDiodeColor(i, (color >> 1) & 0x7F7F7F7F);
    } else { // new fade-to-color algorithm
      int r1 = (color >> 16) & 0xff;
      int g1 = (color >>  8) & 0xff;
      int b1 =  color        & 0xff;

      // calculate the color differences between the current and target colors
      int rdelta = r2 - r1;
      int gdelta = g2 - g1;
      int bdelta = b2 - b1;

      // if the current and target colors are almost the same, jump right to the target color,
      // otherwise calculate an intermediate color. (fixes rounding issues)
      rdelta = abs(rdelta) < 3 ? rdelta : (rdelta >> rateH) + (rdelta >> rateL);
      gdelta = abs(gdelta) < 3 ? gdelta : (gdelta >> rateH) + (gdelta >> rateL);
      bdelta = abs(bdelta) < 3 ? bdelta : (bdelta >> rateH) + (bdelta >> rateL);

      WS2812B_SetDiodeRGB(i, r1 + rdelta, g1 + gdelta, b1 + bdelta);
    }
  }
}

/*
 * color blend function
 */
uint32_t color_blend(uint32_t color1, uint32_t color2, uint8_t blend)
{
  if(blend == 0)   return color1;
  if(blend == 255) return color2;

  int w1 = (color1 >> 24) & 0xff;
  int r1 = (color1 >> 16) & 0xff;
  int g1 = (color1 >>  8) & 0xff;
  int b1 =  color1        & 0xff;

  int w2 = (color2 >> 24) & 0xff;
  int r2 = (color2 >> 16) & 0xff;
  int g2 = (color2 >>  8) & 0xff;
  int b2 =  color2        & 0xff;

  uint32_t w3 = ((w2 * blend) + (w1 * (255 - blend))) / 256;
  uint32_t r3 = ((r2 * blend) + (r1 * (255 - blend))) / 256;
  uint32_t g3 = ((g2 * blend) + (g1 * (255 - blend))) / 256;
  uint32_t b3 = ((b2 * blend) + (b1 * (255 - blend))) / 256;

  return ((w3 << 24) | (r3 << 16) | (g3 << 8) | (b3));
}

/*
 * No blinking. Just plain old static light.
 */
void mode_static(void)
{

  for(uint16_t i = Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++) {
	  WS2812B_SetDiodeColorStruct(i, Ws28b12b_Segments[mActualSegment].ModeColor_w[0]);
  }
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}

//
//	from: 0 - black to color, 1 - white to color
//
void to_color(uint8_t from)
{
	// HSV Saturatioin modifing
	uint16_t h;
	uint8_t s, v, r, g, b;

	WS2812BFX_RGBtoHSV(Ws28b12b_Segments[mActualSegment].ModeColor_w[0].red, Ws28b12b_Segments[mActualSegment].ModeColor_w[0].green, Ws28b12b_Segments[mActualSegment].ModeColor_w[0].blue, &h, &s, &v);

	if(from)
		WS2812BFX_HSVtoRGB(h, s - Ws28b12b_Segments[mActualSegment].CounterModeStep, v, &r, &g, &b);
	else
		WS2812BFX_HSVtoRGB(h, s, v - Ws28b12b_Segments[mActualSegment].CounterModeStep, &r, &g, &b);

	WS2812BFX_SetAllRGB(mActualSegment, r, g, b);

	if(!Ws28b12b_Segments[mActualSegment].Cycle)
	{
		if(from)
		{
			if(Ws28b12b_Segments[mActualSegment].CounterModeStep < s)
				Ws28b12b_Segments[mActualSegment].CounterModeStep++;
			else
				Ws28b12b_Segments[mActualSegment].Cycle = 1;
		}
		else
		{
			if(Ws28b12b_Segments[mActualSegment].CounterModeStep < v)
				Ws28b12b_Segments[mActualSegment].CounterModeStep++;
			else
				Ws28b12b_Segments[mActualSegment].Cycle = 1;
		}
	}
	else
	{
		if(Ws28b12b_Segments[mActualSegment].CounterModeStep > 0)
			Ws28b12b_Segments[mActualSegment].CounterModeStep--;
		else
			Ws28b12b_Segments[mActualSegment].Cycle = 0;
	}

	if(from)
	{
		Ws28b12b_Segments[mActualSegment].ModeDelay = (Ws28b12b_Segments[mActualSegment].Speed / s / 2);
	}
	else
	{
		Ws28b12b_Segments[mActualSegment].ModeDelay = (Ws28b12b_Segments[mActualSegment].Speed / v / 2);
	}
}


void mode_white_to_color(void)
{
	to_color(1);
}

void mode_black_to_color(void)
{
	to_color(0);
}

//
//	Blink helper function
//
void blink(uint32_t color1, uint32_t color2, uint8_t strobe)
{
	uint32_t color = ((Ws28b12b_Segments[mActualSegment].CounterModeCall & 1) == 0) ? color1 : color2;
	WS2812BFX_SetAll(mActualSegment, color);
	if((Ws28b12b_Segments[mActualSegment].CounterModeCall & 1) == 0)
		Ws28b12b_Segments[mActualSegment].ModeDelay = strobe ? 20 : Ws28b12b_Segments[mActualSegment].Speed / 2;
	else
		Ws28b12b_Segments[mActualSegment].ModeDelay = strobe? Ws28b12b_Segments[mActualSegment].Speed - 20 : (Ws28b12b_Segments[mActualSegment].Speed / 2);
}

/*
 * Normal blinking. 50% on/off time.
 */
void mode_blink(void)
{
	blink(Ws28b12b_Segments[mActualSegment].ModeColor[0], Ws28b12b_Segments[mActualSegment].ModeColor[1], 0);
}

void mode_blink_rainbow(void)
{
	blink(color_wheel(Ws28b12b_Segments[mActualSegment].CounterModeCall & 0xFF), Ws28b12b_Segments[mActualSegment].ModeColor[1], 0);
}

void mode_strobe(void) {
	blink(Ws28b12b_Segments[mActualSegment].ModeColor[0], Ws28b12b_Segments[mActualSegment].ModeColor[1], 1);
}

void mode_strobe_rainbow(void)
{
	blink(color_wheel(Ws28b12b_Segments[mActualSegment].CounterModeCall & 0xFF), Ws28b12b_Segments[mActualSegment].ModeColor[1], 1);
}

/*
 * Breathing effect
 */
void mode_breath(void)
{
	uint32_t lum = Ws28b12b_Segments[mActualSegment].CounterModeStep;
	if(lum > 255) lum = 511 - lum;

	uint16_t delay;
	if(lum == 15) delay = 970; // 970 pause before each breath
	else if(lum <=  25) delay = 38; // 19
	else if(lum <=  50) delay = 36; // 18
	else if(lum <=  75) delay = 28; // 14
	else if(lum <= 100) delay = 20; // 10
	else if(lum <= 125) delay = 14; // 7
	else if(lum <= 150) delay = 11; // 5
	else delay = 10; // 4

	uint8_t r = Ws28b12b_Segments[mActualSegment].ModeColor_w[0].red * lum / 256;
	uint8_t g = Ws28b12b_Segments[mActualSegment].ModeColor_w[0].green * lum / 256;
	uint8_t b = Ws28b12b_Segments[mActualSegment].ModeColor_w[0].blue * lum / 256;

	WS2812BFX_SetAllRGB(mActualSegment, r, g, b);
	Ws28b12b_Segments[mActualSegment].CounterModeStep += 2;
	if(Ws28b12b_Segments[mActualSegment].CounterModeStep > (512-15)) Ws28b12b_Segments[mActualSegment].CounterModeStep = 15;
	Ws28b12b_Segments[mActualSegment].ModeDelay = delay;
}

/*
 * Color wipe function
 * LEDs are turned on (color1) in sequence, then turned off (color2) in sequence.
 * if (bool rev == true) then LEDs are turned off in reverse order
 */
void color_wipe(uint32_t color1, uint32_t color2, uint8_t rev)
{
    if(Ws28b12b_Segments[mActualSegment].CounterModeStep < SEGMENT_LENGTH)
    {
    	uint32_t led_offset = Ws28b12b_Segments[mActualSegment].CounterModeStep;
        if(rev)
        {
        	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - led_offset, color1);
        }
        else
        {
        	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + led_offset, color1);
        }
    }
	else
	{
	    uint32_t led_offset = Ws28b12b_Segments[mActualSegment].CounterModeStep - SEGMENT_LENGTH;
        if(rev)
        {
        	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - led_offset, color2);
        }
        else
        {
        	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + led_offset, color2);
        }

    }
    Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % (SEGMENT_LENGTH * 2);
    Ws28b12b_Segments[mActualSegment].ModeDelay =  Ws28b12b_Segments[mActualSegment].Speed;
}

/*
 * Lights all LEDs one after another.
 */
void mode_color_wipe(void)
{
	color_wipe(Ws28b12b_Segments[mActualSegment].ModeColor[0], Ws28b12b_Segments[mActualSegment].ModeColor[1], 0);
}

void mode_color_wipe_inv(void)
{
	color_wipe(Ws28b12b_Segments[mActualSegment].ModeColor[1], Ws28b12b_Segments[mActualSegment].ModeColor[0], 0);
}

void mode_color_wipe_rev(void)
{
	color_wipe(Ws28b12b_Segments[mActualSegment].ModeColor[0], Ws28b12b_Segments[mActualSegment].ModeColor[1], 1);
}

void mode_color_wipe_rev_inv(void)
{
	color_wipe(Ws28b12b_Segments[mActualSegment].ModeColor[1], Ws28b12b_Segments[mActualSegment].ModeColor[0], 1);
}

void mode_color_wipe_random(void)
{
	if(Ws28b12b_Segments[mActualSegment].CounterModeStep % SEGMENT_LENGTH == 0)
	{
	  Ws28b12b_Segments[mActualSegment].AuxParam = get_random_wheel_index(Ws28b12b_Segments[mActualSegment].AuxParam);
	}
	uint32_t color = color_wheel(Ws28b12b_Segments[mActualSegment].AuxParam);

	color_wipe(color, color, 0);
	Ws28b12b_Segments[mActualSegment].ModeDelay =  Ws28b12b_Segments[mActualSegment].Speed;
}

/*
 * Random color introduced alternating from start and end of strip.
 */
void mode_color_sweep_random(void)
{
  if(Ws28b12b_Segments[mActualSegment].CounterModeStep % SEGMENT_LENGTH == 0)
  { // aux_param will store our random color wheel index
	  Ws28b12b_Segments[mActualSegment].AuxParam = get_random_wheel_index(Ws28b12b_Segments[mActualSegment].AuxParam);
  }
  uint32_t color = color_wheel(Ws28b12b_Segments[mActualSegment].AuxParam);
  color_wipe(color, color, 1);
}

/*
 * Lights all LEDs in one random color up. Then switches them
 * to the next random color.
 */
void mode_random_color(void)
{
	Ws28b12b_Segments[mActualSegment].AuxParam = get_random_wheel_index(Ws28b12b_Segments[mActualSegment].AuxParam); // aux_param will store our random color wheel index
	WS2812BFX_SetAll(mActualSegment, color_wheel(Ws28b12b_Segments[mActualSegment].AuxParam));
	Ws28b12b_Segments[mActualSegment].ModeDelay =  Ws28b12b_Segments[mActualSegment].Speed;
}

/*
 * Lights every LED in a random color. Changes one random LED after the other
 * to another random color.
 */
void mode_single_dynamic(void)
{
	if(Ws28b12b_Segments[mActualSegment].CounterModeCall == 0)
	{
		for(uint16_t i = Ws28b12b_Segments[mActualSegment].IdStop; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++)
		{
			WS2812B_SetDiodeColor(i, color_wheel(rand()%256));
		}
	}

	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + rand() % SEGMENT_LENGTH, color_wheel(rand()%256));
	Ws28b12b_Segments[mActualSegment].ModeDelay =  Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Lights every LED in a random color. Changes all LED at the same time
 * to new random colors.
 */
void mode_multi_dynamic(void)
{
	for(uint16_t i = Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++)
	{
		WS2812B_SetDiodeColor(i, color_wheel(rand()%256));
	}
	Ws28b12b_Segments[mActualSegment].ModeDelay =  Ws28b12b_Segments[mActualSegment].Speed;
}

/*
 * Cycles all LEDs at once through a rainbow.
 */
void mode_rainbow(void)
{
  uint32_t color = color_wheel(Ws28b12b_Segments[mActualSegment].CounterModeStep);
  for(uint16_t i = Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++)
  {
	  WS2812B_SetDiodeColor(i, color);
  }

  Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) & 0xFF;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Cycles a rainbow over the entire string of LEDs.
 */
void mode_rainbow_cycle(void)
{
  for(uint16_t i=0; i < SEGMENT_LENGTH; i++)
  {
	  uint32_t color = color_wheel(((i * 256 / SEGMENT_LENGTH) + Ws28b12b_Segments[mActualSegment].CounterModeStep) & 0xFF);
	  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + i, color);
  }

  Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) & 0xFF;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Fades the LEDs between two colors
 */
void mode_fade(void) {
  int lum = Ws28b12b_Segments[mActualSegment].CounterModeStep;
  if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0

  uint32_t color = color_blend(Ws28b12b_Segments[mActualSegment].ModeColor[0], Ws28b12b_Segments[mActualSegment].ModeColor[1], lum);

  for(uint16_t i=Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++) {
    WS2812B_SetDiodeColor(i, color);
  }

  Ws28b12b_Segments[mActualSegment].CounterModeStep += 4;
  if(Ws28b12b_Segments[mActualSegment].CounterModeStep > 511) Ws28b12b_Segments[mActualSegment].CounterModeStep = 0;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Runs a single pixel back and forth.
 */
void mode_scan(void) {
  if(Ws28b12b_Segments[mActualSegment].CounterModeStep > (SEGMENT_LENGTH * 2) - 3) {
    Ws28b12b_Segments[mActualSegment].CounterModeStep = 0;
  }

  for(uint16_t i = Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++) {
    WS2812B_SetDiodeColor(i, Ws28b12b_Segments[mActualSegment].ModeColor[1]);
  }

  int led_offset = Ws28b12b_Segments[mActualSegment].CounterModeStep - (SEGMENT_LENGTH - 1);
  led_offset = abs(led_offset);

  if(IS_REVERSE) {
    WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - led_offset, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
  } else {
    WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + led_offset, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
  }


  Ws28b12b_Segments[mActualSegment].CounterModeStep++;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Runs two pixel back and forth in opposite directions.
 */
void mode_dual_scan(void) {
  if(Ws28b12b_Segments[mActualSegment].CounterModeStep > (SEGMENT_LENGTH * 2) - 3)
  {
    Ws28b12b_Segments[mActualSegment].CounterModeStep = 0;
  }

  for(uint16_t i=Ws28b12b_Segments[mActualSegment].IdStart; i <Ws28b12b_Segments[mActualSegment].IdStop; i++)
  {
    WS2812B_SetDiodeColor(i, Ws28b12b_Segments[mActualSegment].ModeColor[1]);
  }

  int led_offset = Ws28b12b_Segments[mActualSegment].CounterModeStep - (SEGMENT_LENGTH - 1);
  led_offset = abs(led_offset);

  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + led_offset, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + SEGMENT_LENGTH - led_offset - 1, Ws28b12b_Segments[mActualSegment].ModeColor[0]);

  Ws28b12b_Segments[mActualSegment].CounterModeStep++;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}

/*
 * theater chase function
 */
void theater_chase(uint32_t color1, uint32_t color2)
{
	Ws28b12b_Segments[mActualSegment].CounterModeCall = Ws28b12b_Segments[mActualSegment].CounterModeCall % 3;
  for(uint16_t i=0; i < SEGMENT_LENGTH; i++) {
    if((i % 3) == Ws28b12b_Segments[mActualSegment].CounterModeCall) {
      if(IS_REVERSE) {
    	  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - i, color1);
      } else {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + i, color1);
      }
    } else {
      if(IS_REVERSE) {
        WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - i, color2);
      } else {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + i, color2);
      }
    }
  }
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Theatre-style crawling lights.
 * Inspired by the Adafruit examples.
 */
void mode_theater_chase(void)
{

  return theater_chase(Ws28b12b_Segments[mActualSegment].ModeColor[0], Ws28b12b_Segments[mActualSegment].ModeColor[1]);
}


/*
 * Theatre-style crawling lights with rainbow effect.
 * Inspired by the Adafruit examples.
 */
void mode_theater_chase_rainbow(void)
{

	Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) & 0xFF;
	theater_chase(color_wheel(Ws28b12b_Segments[mActualSegment].CounterModeStep), BLACK);
}

/*
 * Running lights effect with smooth sine transition.
 */
void mode_running_lights(void) {
  uint8_t r = ((Ws28b12b_Segments[mActualSegment].ModeColor[0] >> 16) & 0xFF);
  uint8_t g = ((Ws28b12b_Segments[mActualSegment].ModeColor[0] >>  8) & 0xFF);
  uint8_t b =  (Ws28b12b_Segments[mActualSegment].ModeColor[0]        & 0xFF);

  uint8_t sineIncr = MAX(1, (256 / WS2812B_LEDS));
  for(uint16_t i=0; i < SEGMENT_LENGTH; i++) {
    int lum = (int)sine8(((i + Ws28b12b_Segments[mActualSegment].CounterModeStep) * sineIncr));
    if(IS_REVERSE) {
    WS2812B_SetDiodeRGB(Ws28b12b_Segments[mActualSegment].IdStart + i,  (r * lum) / 256, (g * lum) / 256, (b * lum) / 256);
    } else {
    WS2812B_SetDiodeRGB(Ws28b12b_Segments[mActualSegment].IdStop - i,  (r * lum) / 256, (g * lum) / 256, (b * lum) / 256);
    }
  }
  Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % 256;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * twinkle function
 */
void twinkle(uint32_t color1, uint32_t color2)
{
  if(Ws28b12b_Segments[mActualSegment].CounterModeStep == 0)
  {
    for(uint16_t i=Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++)
    {
    	WS2812B_SetDiodeColor(i, color2);
    }
    uint16_t min_leds = MAX(1, WS2812B_LEDS / 5); // make sure, at least one LED is on
    uint16_t max_leds = MAX(1, WS2812B_LEDS / 2); // make sure, at least one LED is on
    Ws28b12b_Segments[mActualSegment].CounterModeStep = rand() % (max_leds + 1 - min_leds) + min_leds;
  }

  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + rand() % SEGMENT_LENGTH, color1);

  Ws28b12b_Segments[mActualSegment].CounterModeStep--;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}

/*
 * Blink several LEDs on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
void mode_twinkle(void)
{
  return twinkle(Ws28b12b_Segments[mActualSegment].ModeColor[0], Ws28b12b_Segments[mActualSegment].ModeColor[1]);
}

/*
 * Blink several LEDs in random colors on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
void mode_twinkle_random(void)
{
  return twinkle(color_wheel(rand() % 256), Ws28b12b_Segments[mActualSegment].ModeColor[1]);
}

/*
 * twinkle_fade function
 */
void twinkle_fade(uint32_t color)
{
  fade_out();

  if((rand() %3) == 0)
  {
	  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + rand() % SEGMENT_LENGTH, color);
  }
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Blink several LEDs on, fading out.
 */
void mode_twinkle_fade(void)
{
  twinkle_fade(Ws28b12b_Segments[mActualSegment].ModeColor[0]);
}


/*
 * Blink several LEDs in random colors on, fading out.
 */
void mode_twinkle_fade_random(void)
{
  twinkle_fade(color_wheel(rand() % 256));
}

/*
 * Blinks one LED at a time.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
void mode_sparkle(void)
{
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + Ws28b12b_Segments[mActualSegment].AuxParam16b, Ws28b12b_Segments[mActualSegment].ModeColor[1]);
  Ws28b12b_Segments[mActualSegment].AuxParam16b = rand() % SEGMENT_LENGTH; // aux_param3 stores the random led index
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + Ws28b12b_Segments[mActualSegment].AuxParam16b, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Lights all LEDs in the color. Flashes single white pixels randomly.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
void mode_flash_sparkle(void)
{
  if(Ws28b12b_Segments[mActualSegment].CounterModeCall == 0)
  {
    for(uint16_t i=Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++)
    {
      WS2812B_SetDiodeColor(i, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
    }
  }

  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + Ws28b12b_Segments[mActualSegment].AuxParam16b, Ws28b12b_Segments[mActualSegment].ModeColor[0]);

  if(rand() % 5 == 0)
  {
    Ws28b12b_Segments[mActualSegment].AuxParam16b = rand() % SEGMENT_LENGTH; // aux_param3 stores the random led index
    WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + Ws28b12b_Segments[mActualSegment].AuxParam16b, WHITE);
    Ws28b12b_Segments[mActualSegment].ModeDelay = 20;
  }
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Like flash sparkle. With more flash.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
void mode_hyper_sparkle(void)
{
  for(uint16_t i=Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++)
  {
    WS2812B_SetDiodeColor(i, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
  }

  if(rand() % 5 < 2)
  {
    for(uint16_t i=0; i < MAX(1, SEGMENT_LENGTH/3); i++)
    {
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + rand() % SEGMENT_LENGTH, WHITE);
    }
    Ws28b12b_Segments[mActualSegment].ModeDelay = 20;
  }
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}

/*
 * Strobe effect with different strobe count and pause, controlled by speed.
 */
void mode_multi_strobe(void)
{
  for(uint16_t i=Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++)
  {
	  WS2812B_SetDiodeColor(i, BLACK);
  }

  uint16_t delay = 200 + ((9 - (Ws28b12b_Segments[mActualSegment].Speed % 10)) * 100);
  uint16_t count = 2 * ((Ws28b12b_Segments[mActualSegment].Speed / 100) + 1);
  if(Ws28b12b_Segments[mActualSegment].CounterModeStep < count)
  {
    if((Ws28b12b_Segments[mActualSegment].CounterModeStep & 1) == 0)
    {
      for(uint16_t i=Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++)
      {
    	  WS2812B_SetDiodeColor(i, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
      }
      delay = 20;
    }
    else
    {
      delay = 50;
    }
  }
  Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % (count + 1);
  Ws28b12b_Segments[mActualSegment].ModeDelay = delay;
}

/*
 * color chase function.
 * color1 = background color
 * color2 and color3 = colors of two adjacent leds
 */
void chase(uint32_t color1, uint32_t color2, uint32_t color3)
{
  uint16_t a = Ws28b12b_Segments[mActualSegment].CounterModeStep;
  uint16_t b = (a + 1) % SEGMENT_LENGTH;
  uint16_t c = (b + 1) % SEGMENT_LENGTH;
  if(IS_REVERSE) {
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - a, color1);
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - b, color2);
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - c, color3);
  } else {
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + a, color1);
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + b, color2);
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + c, color3);
  }

  if(b == 0) Ws28b12b_Segments[mActualSegment].Cycle = 1;
  else Ws28b12b_Segments[mActualSegment].Cycle = 0;

  Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % WS2812B_LEDS;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Bicolor chase mode
 */
void mode_bicolor_chase(void)
{
  return chase(Ws28b12b_Segments[mActualSegment].ModeColor[0], Ws28b12b_Segments[mActualSegment].ModeColor[1], Ws28b12b_Segments[mActualSegment].ModeColor[2]);
}


/*
 * White running on _color.
 */
void mode_chase_color(void)
{
  return chase(Ws28b12b_Segments[mActualSegment].ModeColor[0], WHITE, WHITE);
}


/*
 * Black running on _color.
 */
void mode_chase_blackout(void)
{
  return chase(Ws28b12b_Segments[mActualSegment].ModeColor[0], BLACK, BLACK);
}


/*
 * _color running on white.
 */
void mode_chase_white(void)
{
  return chase(WHITE, Ws28b12b_Segments[mActualSegment].ModeColor[0], Ws28b12b_Segments[mActualSegment].ModeColor[0]);
}


/*
 * White running followed by random color.
 */
void mode_chase_random(void)
{
  if(Ws28b12b_Segments[mActualSegment].CounterModeStep == 0)
  {
    Ws28b12b_Segments[mActualSegment].AuxParam = get_random_wheel_index(Ws28b12b_Segments[mActualSegment].AuxParam);
  }
  return chase(color_wheel(Ws28b12b_Segments[mActualSegment].AuxParam), WHITE, WHITE);
}


/*
 * Rainbow running on white.
 */
void mode_chase_rainbow_white(void)
{
  uint16_t n = Ws28b12b_Segments[mActualSegment].CounterModeStep;
  uint16_t m = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % WS2812B_LEDS;
  uint32_t color2 = color_wheel(((n * 256 / SEGMENT_LENGTH) + (Ws28b12b_Segments[mActualSegment].CounterModeCall & 0xFF)) & 0xFF);
  uint32_t color3 = color_wheel(((m * 256 / SEGMENT_LENGTH) + (Ws28b12b_Segments[mActualSegment].CounterModeCall & 0xFF)) & 0xFF);

  return chase(WHITE, color2, color3);
}


/*
 * White running on rainbow.
 */
void mode_chase_rainbow(void)
{
  uint8_t color_sep = 256 / SEGMENT_LENGTH;
  uint8_t color_index = Ws28b12b_Segments[mActualSegment].CounterModeCall & 0xFF;
  uint32_t color = color_wheel(((Ws28b12b_Segments[mActualSegment].CounterModeStep * color_sep) + color_index) & 0xFF);

  return chase(color, WHITE, WHITE);
}


/*
 * Black running on rainbow.
 */
void mode_chase_blackout_rainbow(void)
{
  uint8_t color_sep = 256 / SEGMENT_LENGTH;
  uint8_t color_index = Ws28b12b_Segments[mActualSegment].CounterModeCall & 0xFF;
  uint32_t color = color_wheel(((Ws28b12b_Segments[mActualSegment].CounterModeStep * color_sep) + color_index) & 0xFF);

  return chase(color, 0, 0);
}

/*
 * White flashes running on _color.
 */
void mode_chase_flash(void)
{
  const static uint8_t flash_count = 4;
  uint8_t flash_step = Ws28b12b_Segments[mActualSegment].CounterModeCall % ((flash_count * 2) + 1);

  for(uint16_t i=Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++)
  {
    WS2812B_SetDiodeColor(i, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
  }

  uint16_t delay = Ws28b12b_Segments[mActualSegment].Speed;
  if(flash_step < (flash_count * 2))
  {
    if(flash_step % 2 == 0)
    {
      uint16_t n = Ws28b12b_Segments[mActualSegment].CounterModeStep;
      uint16_t m = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % SEGMENT_LENGTH;
      if(IS_REVERSE)
      {
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - n, WHITE);
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - m, WHITE);
      }
      else
      {
        WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + n, WHITE);
        WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + m, WHITE);
      }
      delay = 20;
    }
    else
    {
      delay = 30;
    }
  }
  else
  {
    Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % SEGMENT_LENGTH;
  }
  Ws28b12b_Segments[mActualSegment].ModeDelay = delay;
}


/*
 * White flashes running, followed by random color.
 */
void mode_chase_flash_random(void)
{
  const static uint8_t flash_count = 4;
  uint8_t flash_step = Ws28b12b_Segments[mActualSegment].CounterModeCall % ((flash_count * 2) + 1);

  for(uint16_t i=0; i < Ws28b12b_Segments[mActualSegment].CounterModeStep; i++)
  {
    WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + i, color_wheel(Ws28b12b_Segments[mActualSegment].AuxParam));
  }

  uint16_t delay = Ws28b12b_Segments[mActualSegment].Speed;
  if(flash_step < (flash_count * 2))
  {
    uint16_t n = Ws28b12b_Segments[mActualSegment].CounterModeStep;
    uint16_t m = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % SEGMENT_LENGTH;
    if(flash_step % 2 == 0)
    {
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + n, WHITE);
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + m, WHITE);
      delay = 20;
    }
    else
    {
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + n, color_wheel(Ws28b12b_Segments[mActualSegment].AuxParam));
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + m, BLACK);
      delay = 30;
    }
  }
  else
  {
    Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % SEGMENT_LENGTH;

    if(Ws28b12b_Segments[mActualSegment].CounterModeStep == 0)
    {
      Ws28b12b_Segments[mActualSegment].AuxParam = get_random_wheel_index(Ws28b12b_Segments[mActualSegment].AuxParam);
    }
  }
  Ws28b12b_Segments[mActualSegment].ModeDelay = delay;
}


/*
 * Alternating pixels running function.
 */
void running(uint32_t color1, uint32_t color2)
{
  for(uint16_t i=0; i < SEGMENT_LENGTH; i++)
  {
    if((i + Ws28b12b_Segments[mActualSegment].CounterModeStep) % 4 < 2)
    {
      if(IS_REVERSE) {
    	  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + i, color1);
      } else {
        WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - i, color1);
      }
    } else {
      if(IS_REVERSE) {
        WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + i, color1);
      } else {
        WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - i, color2);
      }
    }
  }

  Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) & 0x3;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}

/*
 * Alternating color/white pixels running.
 */
void mode_running_color(void)
{
  return running(Ws28b12b_Segments[mActualSegment].ModeColor[0], WHITE);
}


/*
 * Alternating red/blue pixels running.
 */
void mode_running_red_blue(void)
{
  return running(RED, BLUE);
}


/*
 * Alternating red/green pixels running.
 */
void mode_merry_christmas(void)
{
  return running(RED, GREEN);
}

/*
 * Alternating orange/purple pixels running.
 */
void mode_halloween(void)
{
  return running(PURPLE, ORANGE);
}

/*
 * Random colored pixels running.
 */
void mode_running_random(void) {
  for(uint16_t i=SEGMENT_LENGTH-1; i > 0; i--) {
    if(IS_REVERSE) {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - i, WS2812B_GetColor(Ws28b12b_Segments[mActualSegment].IdStop - i + 1));
    } else {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + i, WS2812B_GetColor(Ws28b12b_Segments[mActualSegment].IdStart + i - 1));
    }
  }

  if(Ws28b12b_Segments[mActualSegment].CounterModeStep == 0)
  {
    Ws28b12b_Segments[mActualSegment].AuxParam = get_random_wheel_index(Ws28b12b_Segments[mActualSegment].AuxParam);
    if(IS_REVERSE) {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop, color_wheel(Ws28b12b_Segments[mActualSegment].AuxParam));
    } else {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart, color_wheel(Ws28b12b_Segments[mActualSegment].AuxParam));
    }
  }

  Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep == 0) ? 1 : 0;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * K.I.T.T.
 */
void mode_larson_scanner(void) {
  fade_out();

  if(Ws28b12b_Segments[mActualSegment].CounterModeStep < SEGMENT_LENGTH)
  {
    if(IS_REVERSE) {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - Ws28b12b_Segments[mActualSegment].CounterModeStep, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
    } else {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + Ws28b12b_Segments[mActualSegment].CounterModeStep, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
    }
  }
  else
  {
    if(IS_REVERSE) {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - ((SEGMENT_LENGTH * 2) - Ws28b12b_Segments[mActualSegment].CounterModeStep) + 2, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
    } else {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + ((SEGMENT_LENGTH * 2) - Ws28b12b_Segments[mActualSegment].CounterModeStep) - 2, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
    }
  }

  if(Ws28b12b_Segments[mActualSegment].CounterModeStep % SEGMENT_LENGTH  == 0) Ws28b12b_Segments[mActualSegment].Cycle = 1;
  else Ws28b12b_Segments[mActualSegment].Cycle = 1;

  Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % ((SEGMENT_LENGTH * 2) - 2);
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Firing comets from one end.
 */
void mode_comet(void) {
  fade_out();

  if(IS_REVERSE) {
	  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - Ws28b12b_Segments[mActualSegment].CounterModeStep, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
  } else {
	  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + Ws28b12b_Segments[mActualSegment].CounterModeStep, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
  }

  Ws28b12b_Segments[mActualSegment].CounterModeStep = (Ws28b12b_Segments[mActualSegment].CounterModeStep + 1) % SEGMENT_LENGTH;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Fireworks function.
 */
void fireworks(uint32_t color) {
  fade_out();

//// set brightness(i) = brightness(i-1)/4 + brightness(i) + brightness(i+1)/4
/*
// the old way, so many calls to the pokey getPixelColor() function made this super slow
  for(uint16_t i=0 + 1; i <WS2812B_LEDS; i++) {
    uint32_t prevLed = (Adafruit_NeoPixel::getPixelColor(i-1) >> 2) & 0x3F3F3F3F;
    uint32_t thisLed = Adafruit_NeoPixel::getPixelColor(i);
    uint32_t nextLed = (Adafruit_NeoPixel::getPixelColor(i+1) >> 2) & 0x3F3F3F3F;
    WS2812B_SetDiodeColor(i, prevLed + thisLed + nextLed);
  }
*/

// the new way, manipulate the Adafruit_NeoPixels pixels[] array directly, about 5x faster
  uint8_t *pixels = WS2812B_GetPixels();
  uint8_t pixelsPerLed = 3;
  uint16_t startPixel = Ws28b12b_Segments[mActualSegment].IdStart * pixelsPerLed + pixelsPerLed;
  uint16_t stopPixel = Ws28b12b_Segments[mActualSegment].IdStop * pixelsPerLed ;
  for(uint16_t i=startPixel; i <stopPixel; i++)
  {
    uint16_t tmpPixel = (pixels[i - pixelsPerLed] >> 2) +
      pixels[i] +
      (pixels[i + pixelsPerLed] >> 2);
    pixels[i] =  tmpPixel > 255 ? 255 : tmpPixel;
  }

  if(!mTriggered)
  {
    for(uint16_t i=0; i<MAX(1, WS2812B_LEDS/20); i++)
    {
      if(rand()%10 == 0)
      {
        WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + rand() % SEGMENT_LENGTH, color);
      }
    }
  }
  else
  {
    for(uint16_t i=0; i<MAX(1, SEGMENT_LENGTH/10); i++)
    {
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + rand() % SEGMENT_LENGTH, color);
    }
  }
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}

/*
 * Firework sparks.
 */
void mode_fireworks(void)
{
  return fireworks(Ws28b12b_Segments[mActualSegment].ModeColor[0]);
}

/*
 * Random colored firework sparks.
 */
void mode_fireworks_random(void)
{
  return fireworks(color_wheel(rand()%256));
}


/*
 * Fire flicker function
 */
void fire_flicker(int rev_intensity)
{
  uint8_t r = (Ws28b12b_Segments[mActualSegment].ModeColor[0] >> 16) & 0xFF;
  uint8_t g = (Ws28b12b_Segments[mActualSegment].ModeColor[0] >>  8) & 0xFF;
  uint8_t b = (Ws28b12b_Segments[mActualSegment].ModeColor[0]        & 0xFF);
  uint8_t lum = MAX(r, MAX(g, b)) / rev_intensity;
  for(uint16_t i=Ws28b12b_Segments[mActualSegment].IdStart; i <= Ws28b12b_Segments[mActualSegment].IdStop; i++)
  {
    int flicker = rand()%lum;
    WS2812B_SetDiodeRGB(i, MAX(r - flicker, 0), MAX(g - flicker, 0), MAX(b - flicker, 0));
  }
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}

/*
 * Random flickering.
 */
void mode_fire_flicker(void)
{
  return fire_flicker(3);
}

/*
* Random flickering, less intensity.
*/
void mode_fire_flicker_soft(void)
{
  return fire_flicker(6);
}

/*
* Random flickering, more intensity.
*/
void mode_fire_flicker_intense(void)
{
  return fire_flicker(1.7);
}


/*
 * Tricolor chase function
 */
void tricolor_chase(uint32_t color1, uint32_t color2, uint32_t color3)
{
  uint16_t index = Ws28b12b_Segments[mActualSegment].CounterModeStep % 6;
  for(uint16_t i=0; i < SEGMENT_LENGTH; i++, index++)
  {
    if(index > 5) index = 0;

    uint32_t color = color3;
    if(index < 2) color = color1;
    else if(index < 4) color = color2;

    if(IS_REVERSE) {
    	WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + i, color);
    } else {
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStop - i, color);
    }
  }

  Ws28b12b_Segments[mActualSegment].CounterModeStep++;
  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}


/*
 * Tricolor chase mode
 */
void mode_tricolor_chase(void)
{
  return tricolor_chase(Ws28b12b_Segments[mActualSegment].ModeColor[0], Ws28b12b_Segments[mActualSegment].ModeColor[1], Ws28b12b_Segments[mActualSegment].ModeColor[2]);
}


/*
 * Alternating white/red/black pixels running.
 */
void mode_circus_combustus(void)
{
  return tricolor_chase(RED, WHITE, BLACK);
}

/*
 * ICU mode
 */
void mode_icu(void)
{
  uint16_t dest = Ws28b12b_Segments[mActualSegment].CounterModeStep & 0xFFFF;

  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + dest, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + dest + WS2812B_LEDS/2, Ws28b12b_Segments[mActualSegment].ModeColor[0]);

  if(Ws28b12b_Segments[mActualSegment].AuxParam16b == dest)
  { // pause between eye movements
    if(rand()%6 == 0)
    { // blink once in a while
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + dest, BLACK);
      WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + dest + SEGMENT_LENGTH/2, BLACK);
      Ws28b12b_Segments[mActualSegment].ModeDelay = 200;
    }
    Ws28b12b_Segments[mActualSegment].AuxParam16b = rand() %(SEGMENT_LENGTH/2);
    Ws28b12b_Segments[mActualSegment].ModeDelay = 1000 + rand() %2000;
  }

  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + dest, BLACK);
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + dest + SEGMENT_LENGTH/2, BLACK);

  if(Ws28b12b_Segments[mActualSegment].AuxParam16b > Ws28b12b_Segments[mActualSegment].CounterModeStep)
  {
    Ws28b12b_Segments[mActualSegment].CounterModeStep++;
    dest++;
  } else if (Ws28b12b_Segments[mActualSegment].AuxParam16b < Ws28b12b_Segments[mActualSegment].CounterModeStep)
  {
    Ws28b12b_Segments[mActualSegment].CounterModeStep--;
    dest--;
  }

  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + dest, Ws28b12b_Segments[mActualSegment].ModeColor[0]);
  WS2812B_SetDiodeColor(Ws28b12b_Segments[mActualSegment].IdStart + dest + SEGMENT_LENGTH/2, Ws28b12b_Segments[mActualSegment].ModeColor[0]);

  Ws28b12b_Segments[mActualSegment].ModeDelay = Ws28b12b_Segments[mActualSegment].Speed;
}
