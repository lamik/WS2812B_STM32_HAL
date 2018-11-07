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

uint8_t		mMode_index = DEFAULT_MODE;
uint16_t	mSpeed = DEFAULT_SPEED;
uint8_t 	mBrightness = DEFAULT_BRIGHTNESS;
uint8_t 	mRunning;
uint8_t 	mTriggered;

volatile uint32_t	iModeDelay;
uint8_t 			mActualMode;
uint32_t			mCounterModeCall;
uint32_t			mCounterModeStep;
unsigned long		mModeLastCallTime;

uint32_t		mColor[NUM_COLORS];
uint32_t		mModeColor[NUM_COLORS];
ws2812b_color	mColor_w[NUM_COLORS];
ws2812b_color	mModeColor_w[NUM_COLORS];

uint8_t 	mAuxParam;
uint16_t 	mAuxParam16b;
uint8_t 	mCycle;

void (*mModeCallback)(void);

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

void WS2812BFX_SysTickCallback(void)
{
	if(iModeDelay > 0) iModeDelay--;
}

void WS2812BFX_Callback(void)
 {
  if(mRunning || mTriggered)
  {
	  if(!iModeDelay)
	  {
		  mModeCallback();
		  WS2812B_Refresh();
		  mCounterModeCall++;
	  }
  }
}

void WS2812BFX_SetMode(fx_mode mode)
{
	mCounterModeCall = 0;
	mCounterModeStep = 0;
	mModeLastCallTime = 0;
	mActualMode = mode;
	mCycle = 0;
	mModeCallback = mMode[mActualMode];
	for(uint8_t i = 0; i < NUM_COLORS; i++)
	{
		mModeColor[i] = mColor[i];
		mModeColor_w[i].red = mColor_w[i].red;
		mModeColor_w[i].green = mColor_w[i].green;
		mModeColor_w[i].blue = mColor_w[i].blue;
	}
}

void WS2812BFX_NextMode(void)
{
	mCounterModeCall = 0;
	mCounterModeStep = 0;
	mModeLastCallTime = 0;
	mActualMode++;
	mCycle = 0;
	if(mActualMode >= MODE_COUNT) mActualMode = 0;
	mModeCallback = mMode[mActualMode];
	for(uint8_t i = 0; i < NUM_COLORS; i++)
	{
		mModeColor[i] = mColor[i];
		mModeColor_w[i].red = mColor_w[i].red;
		mModeColor_w[i].green = mColor_w[i].green;
		mModeColor_w[i].blue = mColor_w[i].blue;
	}
}

void WS2812BFX_PrevMode(void)
{
	mCounterModeCall = 0;
	mCounterModeStep = 0;
	mModeLastCallTime = 0;
	mCycle = 0;
	if(mActualMode == 0) mActualMode = MODE_COUNT;
	else mActualMode--;
	mModeCallback = mMode[mActualMode];
	for(uint8_t i = 0; i < NUM_COLORS; i++)
	{
		mModeColor[i] = mColor[i];
		mModeColor_w[i].red = mColor_w[i].red;
		mModeColor_w[i].green = mColor_w[i].green;
		mModeColor_w[i].blue = mColor_w[i].blue;
	}
}

void WS2812BFX_Start()
{
	mCounterModeCall = 0;
	mCounterModeStep = 0;
	mModeLastCallTime = 0;
	iModeDelay = 0;
	mRunning = 1;
}

void WS2812BFX_Stop()
{
	mRunning = 0;
//	  strip_off();
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

void WS2812BFX_SetAll(uint32_t c)
{

	for(uint16_t i = 0; i < WS2812B_LEDS; i++)
	{
		WS2812B_SetDiodeRGB(i, ((c>>16)&0xFF), ((c>>8)&0xFF), (c&0xFF));
	}
}

void WS2812BFX_SetAllRGB(uint8_t r, uint8_t g, uint8_t b)
{
	for(uint16_t i = 0; i < WS2812B_LEDS; i++)
	{
		WS2812B_SetDiodeRGB(i, r, g, b);
	}
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
		q = (v * (255 - (s * Fracts)/360)) / 256;
		t = (v * (255 - (s * (360 - Fracts))/360)) / 256;


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

uint8_t WS2812BFX_IsRunning()
{
  return mRunning;
}

uint8_t WS2812BFX_GetMode(void)
{
  return mMode_index;
}

uint8_t WS2812BFX_GetSpeed(void)
{
  return mSpeed;
}

uint8_t WS2812BFX_GetBrightness(void)
{
  return mBrightness;
}

uint16_t WS2812BFX_GetLength(void)
{
  return WS2812B_LEDS;
}

uint8_t WS2812BFX_GetModeCount(void)
{
  return MODE_COUNT;
}

ws2812b_color WS2812BFX_GetColorStruct(uint8_t id)
{
  return mColor_w[id];
}

uint32_t WS2812BFX_GetColor(uint8_t id)
{
  return mColor[id];
}

void WS2812BFX_SetSpeed(uint16_t Speed)
{
	if(Speed < SPEED_MIN) Speed = SPEED_MIN;
	if(Speed > SPEED_MAX) Speed = SPEED_MAX;

	mSpeed = Speed;
}

void WS2812BFX_IncreaseSpeed(uint16_t Speed)
{
	WS2812BFX_SetSpeed(mSpeed + Speed);
}

void WS2812BFX_DecreaseSpeed(uint16_t Speed)
{
	WS2812BFX_SetSpeed(mSpeed - Speed);
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

  uint32_t color = mModeColor[1]; // target color
  int r2 = (color >> 16) & 0xff;
  int g2 = (color >>  8) & 0xff;
  int b2 =  color        & 0xff;

  for(uint16_t i=0; i <= WS2812B_LEDS; i++) {
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
void mode_static(void) {
  for(uint16_t i=0; i < WS2812B_LEDS; i++) {
	  WS2812B_SetDiodeColorStruct(i, mModeColor_w[0]);
  }
  iModeDelay = 250;
}

//
//	from: 0 - black to color, 1 - white to color
//
void to_color(uint8_t from)
{
	// HSV Saturatioin modifing
	uint16_t h;
	uint8_t s, v, r, g, b;

	WS2812BFX_RGBtoHSV(mModeColor_w[0].red, mModeColor_w[0].green, mModeColor_w[0].blue, &h, &s, &v);

	if(from)
		WS2812BFX_HSVtoRGB(h, s - mCounterModeStep, v, &r, &g, &b);
	else
		WS2812BFX_HSVtoRGB(h, s, v - mCounterModeStep, &r, &g, &b);

	for(uint16_t i = 0; i < WS2812B_LEDS; i++)
	{
		WS2812B_SetDiodeRGB(i, r, g, b);
	}

	if(!mCycle)
	{
		if(from)
		{
			if(mCounterModeStep < s)
				mCounterModeStep++;
			else
				mCycle = 1;
		}
		else
		{
			if(mCounterModeStep < v)
				mCounterModeStep++;
			else
				mCycle = 1;
		}
	}
	else
	{
		if(mCounterModeStep > 0)
			mCounterModeStep--;
		else
			mCycle = 0;
	}

	if(from)
	{
		iModeDelay = mSpeed / s / 2;
	}
	else
	{
		iModeDelay = mSpeed / v / 2;
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
	uint32_t color = ((mCounterModeCall & 1) == 0) ? color1 : color2;
	WS2812BFX_SetAll(color);
	if((mCounterModeCall & 1) == 0)
		iModeDelay = strobe ? 20 : mSpeed / 2;
	else
		iModeDelay = strobe? mSpeed - 20 : (mSpeed / 2);
}

/*
 * Normal blinking. 50% on/off time.
 */
void mode_blink(void) {
	blink(mModeColor[0], mModeColor[1], 0);
}

void mode_blink_rainbow(void)
{
	blink(color_wheel(mCounterModeCall & 0xFF), mModeColor[1], 0);
}

void mode_strobe(void) {
	blink(mModeColor[0], mModeColor[1], 1);
}

void mode_strobe_rainbow(void)
{
	blink(color_wheel(mCounterModeCall & 0xFF), mModeColor[1], 1);
}

/*
 * Breathing effect
 */
void mode_breath(void)
{
	uint32_t lum = mCounterModeStep;
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

	uint8_t r = mModeColor_w[0].red * lum / 256;
	uint8_t g = mModeColor_w[0].green * lum / 256;
	uint8_t b = mModeColor_w[0].blue * lum / 256;

	WS2812BFX_SetAllRGB(r, g, b);
	mCounterModeStep += 2;
	if(mCounterModeStep > (512-15)) mCounterModeStep = 15;
	iModeDelay = delay;
}

/*
 * Color wipe function
 * LEDs are turned on (color1) in sequence, then turned off (color2) in sequence.
 * if (bool rev == true) then LEDs are turned off in reverse order
 */
void color_wipe(uint32_t color1, uint32_t color2, uint8_t rev)
{
    if(mCounterModeStep < WS2812B_LEDS)
    {
    	uint32_t led_offset = mCounterModeStep;
        if(rev)
        {
        	WS2812B_SetDiodeColor(WS2812B_LEDS - led_offset, color1);
        }
        else
        {
        	WS2812B_SetDiodeColor(0 + led_offset, color1);
        }
    }
	else
	{
	    uint32_t led_offset = mCounterModeStep - WS2812B_LEDS;
        if(rev)
        {
        	WS2812B_SetDiodeColor(WS2812B_LEDS - led_offset, color2);
        }
        else
        {
        	WS2812B_SetDiodeColor(0 + led_offset, color2);
        }

    }
    mCounterModeStep = (mCounterModeStep + 1) % (WS2812B_LEDS * 2);
    iModeDelay =  mSpeed;
}

/*
 * Lights all LEDs one after another.
 */
void mode_color_wipe(void)
{
	color_wipe(mModeColor[0], mModeColor[1], 0);
}

void mode_color_wipe_inv(void)
{
	color_wipe(mModeColor[1], mModeColor[0], 0);
}

void mode_color_wipe_rev(void)
{
	color_wipe(mModeColor[0], mModeColor[1], 1);
}

void mode_color_wipe_rev_inv(void)
{
	color_wipe(mModeColor[1], mModeColor[0], 1);
}

void mode_color_wipe_random(void)
	{
	if(mCounterModeStep % WS2812B_LEDS == 0)
	{
	  mAuxParam = get_random_wheel_index(mAuxParam);
	}
	uint32_t color = color_wheel(mAuxParam);

	color_wipe(color, color, 0);
	iModeDelay =  mSpeed;
}

/*
 * Random color introduced alternating from start and end of strip.
 */
void mode_color_sweep_random(void)
{
  if(mCounterModeStep % WS2812B_LEDS == 0)
  { // aux_param will store our random color wheel index
	  mAuxParam = get_random_wheel_index(mAuxParam);
  }
  uint32_t color = color_wheel(mAuxParam);
  color_wipe(color, color, 1);
}

/*
 * Lights all LEDs in one random color up. Then switches them
 * to the next random color.
 */
void mode_random_color(void)
{
	mAuxParam = get_random_wheel_index(mAuxParam); // aux_param will store our random color wheel index
	WS2812BFX_SetAll(color_wheel(mAuxParam));
	iModeDelay =  mSpeed;
}

/*
 * Lights every LED in a random color. Changes one random LED after the other
 * to another random color.
 */
void mode_single_dynamic(void)
{
	if(mCounterModeCall == 0)
	{
		for(uint16_t i = 0; i < WS2812B_LEDS; i++)
		{
			WS2812B_SetDiodeColor(i, color_wheel(rand()%256));
		}
	}

	WS2812B_SetDiodeColor(0 + rand() % WS2812B_LEDS, color_wheel(rand()%256));
	iModeDelay =  mSpeed;
}


/*
 * Lights every LED in a random color. Changes all LED at the same time
 * to new random colors.
 */
void mode_multi_dynamic(void)
{
	for(uint16_t i = 0; i < WS2812B_LEDS; i++)
	{
		WS2812B_SetDiodeColor(i, color_wheel(rand()%256));
	}
	iModeDelay =  mSpeed;
}

/*
 * Cycles all LEDs at once through a rainbow.
 */
void mode_rainbow(void)
{
  uint32_t color = color_wheel(mCounterModeStep);
  for(uint16_t i = 0; i < WS2812B_LEDS; i++)
  {
	  WS2812B_SetDiodeColor(i, color);
  }

  mCounterModeStep = (mCounterModeStep + 1) & 0xFF;
  iModeDelay = mSpeed;
}


/*
 * Cycles a rainbow over the entire string of LEDs.
 */
void mode_rainbow_cycle(void)
{
  for(uint16_t i=0; i < WS2812B_LEDS; i++)
  {
	  uint32_t color = color_wheel(((i * 256 / WS2812B_LEDS) + mCounterModeStep) & 0xFF);
	  WS2812B_SetDiodeColor(0 + i, color);
  }

  mCounterModeStep = (mCounterModeStep + 1) & 0xFF;
  iModeDelay = mSpeed;
}


/*
 * Fades the LEDs between two colors
 */
void mode_fade(void) {
  int lum = mCounterModeStep;
  if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0

  uint32_t color = color_blend(mModeColor[0], mModeColor[1], lum);

  for(uint16_t i=0; i <WS2812B_LEDS; i++) {
    WS2812B_SetDiodeColor(i, color);
  }

  mCounterModeStep += 4;
  if(mCounterModeStep > 511) mCounterModeStep = 0;
  iModeDelay = mSpeed;
}


/*
 * Runs a single pixel back and forth.
 */
void mode_scan(void) {
  if(mCounterModeStep > (WS2812B_LEDS * 2) - 3) {
    mCounterModeStep = 0;
  }

  for(uint16_t i=0; i <WS2812B_LEDS; i++) {
    WS2812B_SetDiodeColor(i, mModeColor[1]);
  }

  int led_offset = mCounterModeStep - (WS2812B_LEDS - 1);
  led_offset = abs(led_offset);


   WS2812B_SetDiodeColor(0 + led_offset, mModeColor[0]);


  mCounterModeStep++;
  iModeDelay = mSpeed;
}


/*
 * Runs two pixel back and forth in opposite directions.
 */
void mode_dual_scan(void) {
  if(mCounterModeStep > (WS2812B_LEDS * 2) - 3) {
    mCounterModeStep = 0;
  }

  for(uint16_t i=0; i <WS2812B_LEDS; i++) {
    WS2812B_SetDiodeColor(i, mModeColor[1]);
  }

  int led_offset = mCounterModeStep - (WS2812B_LEDS - 1);
  led_offset = abs(led_offset);

  WS2812B_SetDiodeColor(0 + led_offset, mModeColor[0]);
  WS2812B_SetDiodeColor(0 + WS2812B_LEDS - led_offset - 1, mModeColor[0]);

  mCounterModeStep++;
  iModeDelay = mSpeed;
}

/*
 * theater chase function
 */
void theater_chase(uint32_t color1, uint32_t color2)
{
	mCounterModeCall = mCounterModeCall % 3;
  for(uint16_t i=0; i < WS2812B_LEDS; i++) {
    if((i % 3) == mCounterModeCall) {
//      if(IS_REVERSE) {
//        WS2812B_SetDiodeColor(WS2812B_LEDS - i, color1);
//      } else {
    	WS2812B_SetDiodeColor(0 + i, color1);
//      }
    } else {
//      if(IS_REVERSE) {
//        WS2812B_SetDiodeColor(WS2812B_LEDS - i, color2);
//      } else {
    	WS2812B_SetDiodeColor(0 + i, color2);
//      }
    }
  }

  iModeDelay = mSpeed;
}


/*
 * Theatre-style crawling lights.
 * Inspired by the Adafruit examples.
 */
void mode_theater_chase(void)
{
  return theater_chase(mModeColor[0], mModeColor[1]);
}


/*
 * Theatre-style crawling lights with rainbow effect.
 * Inspired by the Adafruit examples.
 */
void mode_theater_chase_rainbow(void)
{
	mCounterModeStep = (mCounterModeStep + 1) & 0xFF;
	theater_chase(color_wheel(mCounterModeStep), BLACK);
}

/*
 * Running lights effect with smooth sine transition.
 */
void mode_running_lights(void) {
  uint8_t r = ((mModeColor[0] >> 16) & 0xFF);
  uint8_t g = ((mModeColor[0] >>  8) & 0xFF);
  uint8_t b =  (mModeColor[0]        & 0xFF);

  uint8_t sineIncr = MAX(1, (256 / WS2812B_LEDS));
  for(uint16_t i=0; i < WS2812B_LEDS; i++) {
    int lum = (int)sine8(((i + mCounterModeStep) * sineIncr));
//    if(IS_REVERSE) {
//      WS2812B_SetDiodeColor(0 + i, (r * lum) / 256, (g * lum) / 256, (b * lum) / 256, (w * lum) / 256);
//    } else {
    WS2812B_SetDiodeRGB(WS2812B_LEDS - i,  (r * lum) / 256, (g * lum) / 256, (b * lum) / 256);
//    }
  }
  mCounterModeStep = (mCounterModeStep + 1) % 256;
  iModeDelay = mSpeed;
}


/*
 * twinkle function
 */
void twinkle(uint32_t color1, uint32_t color2)
{
  if(mCounterModeStep == 0)
  {
    for(uint16_t i=0; i <= WS2812B_LEDS; i++)
    {
    	WS2812B_SetDiodeColor(i, color2);
    }
    uint16_t min_leds = MAX(1, WS2812B_LEDS / 5); // make sure, at least one LED is on
    uint16_t max_leds = MAX(1, WS2812B_LEDS / 2); // make sure, at least one LED is on
    mCounterModeStep = rand() % (max_leds + 1 - min_leds) + min_leds;
  }

  WS2812B_SetDiodeColor(0 + rand() % WS2812B_LEDS, color1);

  mCounterModeStep--;
  iModeDelay = mSpeed;
}

/*
 * Blink several LEDs on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
void mode_twinkle(void) {
  return twinkle(mModeColor[0], mModeColor[1]);
}

/*
 * Blink several LEDs in random colors on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
void mode_twinkle_random(void) {
  return twinkle(color_wheel(rand() % 256), mModeColor[1]);
}

/*
 * twinkle_fade function
 */
void twinkle_fade(uint32_t color)
{
  fade_out();

  if((rand() %3) == 0)
  {
	  WS2812B_SetDiodeColor(0 + rand() % WS2812B_LEDS, color);
  }
  iModeDelay = mSpeed;
}


/*
 * Blink several LEDs on, fading out.
 */
void mode_twinkle_fade(void)
{
  twinkle_fade(mModeColor[0]);
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
  WS2812B_SetDiodeColor(0 + mAuxParam16b, mModeColor[1]);
  mAuxParam16b = rand() % WS2812B_LEDS; // aux_param3 stores the random led index
  WS2812B_SetDiodeColor(0 + mAuxParam16b, mModeColor[0]);
  iModeDelay = mSpeed;
}


/*
 * Lights all LEDs in the color. Flashes single white pixels randomly.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
void mode_flash_sparkle(void) {
  if(mCounterModeCall == 0) {
    for(uint16_t i=0; i <= WS2812B_LEDS; i++)
    {
      WS2812B_SetDiodeColor(i, mModeColor[0]);
    }
  }

  WS2812B_SetDiodeColor(0 + mAuxParam16b, mModeColor[0]);

  if(rand() % 5 == 0)
  {
    mAuxParam16b = rand() % WS2812B_LEDS; // aux_param3 stores the random led index
    WS2812B_SetDiodeColor(0 + mAuxParam16b, WHITE);
    iModeDelay = 20;
  }
  iModeDelay = mSpeed;
}


/*
 * Like flash sparkle. With more flash.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
void mode_hyper_sparkle(void)
{
  for(uint16_t i=0; i <= WS2812B_LEDS; i++)
  {
    WS2812B_SetDiodeColor(i, mModeColor[0]);
  }

  if(rand() % 5 < 2)
  {
    for(uint16_t i=0; i < MAX(1, WS2812B_LEDS/3); i++)
    {
      WS2812B_SetDiodeColor(0 + rand() % WS2812B_LEDS, WHITE);
    }
    iModeDelay = 20;
  }
  iModeDelay = mSpeed;
}

/*
 * Strobe effect with different strobe count and pause, controlled by speed.
 */
void mode_multi_strobe(void)
{
  for(uint16_t i=0; i <= WS2812B_LEDS; i++)
  {
	  WS2812B_SetDiodeColor(i, BLACK);
  }

  uint16_t delay = 200 + ((9 - (mSpeed % 10)) * 100);
  uint16_t count = 2 * ((mSpeed / 100) + 1);
  if(mCounterModeStep < count)
  {
    if((mCounterModeStep & 1) == 0)
    {
      for(uint16_t i=0; i <= WS2812B_LEDS; i++)
      {
    	  WS2812B_SetDiodeColor(i, mModeColor[0]);
      }
      delay = 20;
    }
    else
    {
      delay = 50;
    }
  }
  mCounterModeStep = (mCounterModeStep + 1) % (count + 1);
  iModeDelay = delay;
}

/*
 * color chase function.
 * color1 = background color
 * color2 and color3 = colors of two adjacent leds
 */
void chase(uint32_t color1, uint32_t color2, uint32_t color3)
{
  uint16_t a = mCounterModeStep;
  uint16_t b = (a + 1) % WS2812B_LEDS;
  uint16_t c = (b + 1) % WS2812B_LEDS;
//  if(IS_REVERSE) {
//    WS2812B_SetDiodeColor(WS2812B_LEDS - a, color1);
//    WS2812B_SetDiodeColor(WS2812B_LEDS - b, color2);
//    WS2812B_SetDiodeColor(WS2812B_LEDS - c, color3);
//  } else {
  WS2812B_SetDiodeColor(0 + a, color1);
  WS2812B_SetDiodeColor(0 + b, color2);
  WS2812B_SetDiodeColor(0 + c, color3);
//  }

  if(b == 0) mCycle = 1;
  else mCycle = 0;

  mCounterModeStep = (mCounterModeStep + 1) % WS2812B_LEDS;
  iModeDelay = mSpeed;
}


/*
 * Bicolor chase mode
 */
void mode_bicolor_chase(void)
{
  return chase(mModeColor[0], mModeColor[1], mModeColor[2]);
}


/*
 * White running on _color.
 */
void mode_chase_color(void)
{
  return chase(mModeColor[0], WHITE, WHITE);
}


/*
 * Black running on _color.
 */
void mode_chase_blackout(void)
{
  return chase(mModeColor[0], BLACK, BLACK);
}


/*
 * _color running on white.
 */
void mode_chase_white(void)
{
  return chase(WHITE, mModeColor[0], mModeColor[0]);
}


/*
 * White running followed by random color.
 */
void mode_chase_random(void)
{
  if(mCounterModeStep == 0)
  {
    mAuxParam = get_random_wheel_index(mAuxParam);
  }
  return chase(color_wheel(mAuxParam), WHITE, WHITE);
}


/*
 * Rainbow running on white.
 */
void mode_chase_rainbow_white(void)
{
  uint16_t n = mCounterModeStep;
  uint16_t m = (mCounterModeStep + 1) % WS2812B_LEDS;
  uint32_t color2 = color_wheel(((n * 256 / WS2812B_LEDS) + (mCounterModeCall & 0xFF)) & 0xFF);
  uint32_t color3 = color_wheel(((m * 256 / WS2812B_LEDS) + (mCounterModeCall & 0xFF)) & 0xFF);

  return chase(WHITE, color2, color3);
}


/*
 * White running on rainbow.
 */
void mode_chase_rainbow(void)
{
  uint8_t color_sep = 256 / WS2812B_LEDS;
  uint8_t color_index = mCounterModeCall & 0xFF;
  uint32_t color = color_wheel(((mCounterModeStep * color_sep) + color_index) & 0xFF);

  return chase(color, WHITE, WHITE);
}


/*
 * Black running on rainbow.
 */
void mode_chase_blackout_rainbow(void)
{
  uint8_t color_sep = 256 / WS2812B_LEDS;
  uint8_t color_index = mCounterModeCall & 0xFF;
  uint32_t color = color_wheel(((mCounterModeStep * color_sep) + color_index) & 0xFF);

  return chase(color, 0, 0);
}

/*
 * White flashes running on _color.
 */
void mode_chase_flash(void)
{
  const static uint8_t flash_count = 4;
  uint8_t flash_step = mCounterModeCall % ((flash_count * 2) + 1);

  for(uint16_t i=0; i <= WS2812B_LEDS; i++)
  {
    WS2812B_SetDiodeColor(i, mModeColor[0]);
  }

  uint16_t delay = mSpeed;
  if(flash_step < (flash_count * 2))
  {
    if(flash_step % 2 == 0)
    {
      uint16_t n = mCounterModeStep;
      uint16_t m = (mCounterModeStep + 1) % WS2812B_LEDS;
//      if(IS_REVERSE)
//      {
//        WS2812B_SetDiodeColor(WS2812B_LEDS - n, WHITE);
//        WS2812B_SetDiodeColor(WS2812B_LEDS - m, WHITE);
//      }
//      else
//      {
        WS2812B_SetDiodeColor(0 + n, WHITE);
        WS2812B_SetDiodeColor(0 + m, WHITE);
//      }
      delay = 20;
    }
    else
    {
      delay = 30;
    }
  }
  else
  {
    mCounterModeStep = (mCounterModeStep + 1) % WS2812B_LEDS;
  }
  iModeDelay = delay;
}


/*
 * White flashes running, followed by random color.
 */
void mode_chase_flash_random(void)
{
  const static uint8_t flash_count = 4;
  uint8_t flash_step = mCounterModeCall % ((flash_count * 2) + 1);

  for(uint16_t i=0; i < mCounterModeStep; i++)
  {
    WS2812B_SetDiodeColor(0 + i, color_wheel(mAuxParam));
  }

  uint16_t delay = mSpeed;
  if(flash_step < (flash_count * 2))
  {
    uint16_t n = mCounterModeStep;
    uint16_t m = (mCounterModeStep + 1) % WS2812B_LEDS;
    if(flash_step % 2 == 0)
    {
      WS2812B_SetDiodeColor(0 + n, WHITE);
      WS2812B_SetDiodeColor(0 + m, WHITE);
      delay = 20;
    }
    else
    {
      WS2812B_SetDiodeColor(0 + n, color_wheel(mAuxParam));
      WS2812B_SetDiodeColor(0 + m, BLACK);
      delay = 30;
    }
  }
  else
  {
    mCounterModeStep = (mCounterModeStep + 1) % WS2812B_LEDS;

    if(mCounterModeStep == 0)
    {
      mAuxParam = get_random_wheel_index(mAuxParam);
    }
  }
  iModeDelay = delay;
}


/*
 * Alternating pixels running function.
 */
void running(uint32_t color1, uint32_t color2)
{
  for(uint16_t i=0; i < WS2812B_LEDS; i++)
  {
    if((i + mCounterModeStep) % 4 < 2)
    {
//      if(IS_REVERSE) {
//        WS2812B_SetDiodeColor(0 + i, color1);
//      } else {
        WS2812B_SetDiodeColor(WS2812B_LEDS - i, color1);
//      }
    } else {
//      if(IS_REVERSE) {
//        WS2812B_SetDiodeColor(0 + i, color2);
//      } else {
        WS2812B_SetDiodeColor(WS2812B_LEDS - i, color2);
//      }
    }
  }

  mCounterModeStep = (mCounterModeStep + 1) & 0x3;
  iModeDelay = mSpeed;
}

/*
 * Alternating color/white pixels running.
 */
void mode_running_color(void)
{
  return running(mModeColor[0], WHITE);
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
  for(uint16_t i=WS2812B_LEDS-1; i > 0; i--) {
//    if(IS_REVERSE) {
//      WS2812B_SetDiodeColor(WS2812B_LEDS - i, Adafruit_NeoPixel::getPixelColor(WS2812B_LEDS - i + 1));
//    } else {
      WS2812B_SetDiodeColor(0 + i, WS2812B_GetColor(0 + i - 1));
//    }
  }

  if(mCounterModeStep == 0)
  {
    mAuxParam = get_random_wheel_index(mAuxParam);
//    if(IS_REVERSE) {
//      WS2812B_SetDiodeColor(WS2812B_LEDS, color_wheel(mAuxParam));
//    } else {
      WS2812B_SetDiodeColor(0, color_wheel(mAuxParam));
//    }
  }

  mCounterModeStep = (mCounterModeStep == 0) ? 1 : 0;
  iModeDelay = mSpeed;
}


/*
 * K.I.T.T.
 */
void mode_larson_scanner(void) {
  fade_out();

  if(mCounterModeStep < WS2812B_LEDS)
  {
//    if(IS_REVERSE) {
//      WS2812B_SetDiodeColor(WS2812B_LEDS - mCounterModeStep, mModeColor[0]);
//    } else {
      WS2812B_SetDiodeColor(0 + mCounterModeStep, mModeColor[0]);
//    }
  }
  else
  {
//    if(IS_REVERSE) {
//      WS2812B_SetDiodeColor(WS2812B_LEDS - ((WS2812B_LEDS * 2) - mCounterModeStep) + 2, mModeColor[0]);
//    } else {
      WS2812B_SetDiodeColor(0 + ((WS2812B_LEDS * 2) - mCounterModeStep) - 2, mModeColor[0]);
//    }
  }

  if(mCounterModeStep % WS2812B_LEDS == 0) mCycle = 1;
  else mCycle = 1;

  mCounterModeStep = (mCounterModeStep + 1) % ((WS2812B_LEDS * 2) - 2);
  iModeDelay = mSpeed;
}


/*
 * Firing comets from one end.
 */
void mode_comet(void) {
  fade_out();

//  if(IS_REVERSE) {
//    WS2812B_SetDiodeColor(WS2812B_LEDS - mCounterModeStep, mModeColor[0]);
//  } else {
    WS2812B_SetDiodeColor(0 + mCounterModeStep, mModeColor[0]);
//  }

  mCounterModeStep = (mCounterModeStep + 1) % WS2812B_LEDS;
  iModeDelay = mSpeed;
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
  uint16_t startPixel = 0 * pixelsPerLed + pixelsPerLed;
  uint16_t stopPixel = WS2812B_LEDS * pixelsPerLed ;
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
        WS2812B_SetDiodeColor(0 + rand() % WS2812B_LEDS, color);
      }
    }
  }
  else
  {
    for(uint16_t i=0; i<MAX(1, WS2812B_LEDS/10); i++)
    {
      WS2812B_SetDiodeColor(0 + rand() % WS2812B_LEDS, color);
    }
  }
  iModeDelay = mSpeed;
}

/*
 * Firework sparks.
 */
void mode_fireworks(void)
{
  return fireworks(mModeColor[0]);
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
  uint8_t r = (mModeColor[0] >> 16) & 0xFF;
  uint8_t g = (mModeColor[0] >>  8) & 0xFF;
  uint8_t b = (mModeColor[0]        & 0xFF);
  uint8_t lum = MAX(r, MAX(g, b)) / rev_intensity;
  for(uint16_t i=0; i <= WS2812B_LEDS; i++)
  {
    int flicker = rand()%lum;
    WS2812B_SetDiodeRGB(i, MAX(r - flicker, 0), MAX(g - flicker, 0), MAX(b - flicker, 0));
  }
  iModeDelay = mSpeed;
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
  uint16_t index = mCounterModeStep % 6;
  for(uint16_t i=0; i < WS2812B_LEDS; i++, index++)
  {
    if(index > 5) index = 0;

    uint32_t color = color3;
    if(index < 2) color = color1;
    else if(index < 4) color = color2;

//    if(IS_REVERSE) {
//      WS2812B_SetDiodeColor(0 + i, color);
//    } else {
      WS2812B_SetDiodeColor(WS2812B_LEDS - i, color);
//    }
  }

  mCounterModeStep++;
  iModeDelay = mSpeed;
}


/*
 * Tricolor chase mode
 */
void mode_tricolor_chase(void)
{
  return tricolor_chase(mModeColor[0], mModeColor[1], mModeColor[2]);
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
  uint16_t dest = mCounterModeStep & 0xFFFF;

  WS2812B_SetDiodeColor(0 + dest, mModeColor[0]);
  WS2812B_SetDiodeColor(0 + dest + WS2812B_LEDS/2, mModeColor[0]);

  if(mAuxParam16b == dest)
  { // pause between eye movements
    if(rand()%6 == 0)
    { // blink once in a while
      WS2812B_SetDiodeColor(0 + dest, BLACK);
      WS2812B_SetDiodeColor(0 + dest + WS2812B_LEDS/2, BLACK);
      iModeDelay = 200;
    }
    mAuxParam16b = rand() %(WS2812B_LEDS/2);
    iModeDelay = 1000 + rand() %2000;
  }

  WS2812B_SetDiodeColor(0 + dest, BLACK);
  WS2812B_SetDiodeColor(0 + dest + WS2812B_LEDS/2, BLACK);

  if(mAuxParam16b > mCounterModeStep)
  {
    mCounterModeStep++;
    dest++;
  } else if (mAuxParam16b < mCounterModeStep)
  {
    mCounterModeStep--;
    dest--;
  }

  WS2812B_SetDiodeColor(0 + dest, mModeColor[0]);
  WS2812B_SetDiodeColor(0 + dest + WS2812B_LEDS/2, mModeColor[0]);

  iModeDelay = mSpeed;
}
