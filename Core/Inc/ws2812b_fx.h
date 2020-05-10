/*
 * ws2812b_fx.h
 *
 *	Library based on Harm Aldick's Arduino library
 *	https://github.com/kitesurfer1404/WS2812FX
 *
 *	The MIT License.
 *	Created on: 20.10.2018
 *		Author: Mateusz Salamon
 *		www.msalamon.pl
 *		mateusz@msalamon.pl
 */

#ifndef WS2812B_FX_H_
#define WS2812B_FX_H_

#define DEFAULT_COLOR 	0x00FF000000
#define NUM_COLORS		3

#define SPEED_MIN 		10
#define DEFAULT_SPEED 	150
#define SPEED_MAX 		65535

#define MODE_COUNT 		58
#define DEFAULT_MODE 	0

#define FADE_RATE	2

// some common colors
#define RED        (uint32_t)0xFF0000
#define GREEN      (uint32_t)0x00FF00
#define BLUE       (uint32_t)0x0000FF
#define WHITE      (uint32_t)0xFFFFFF
#define BLACK      (uint32_t)0x000000
#define YELLOW     (uint32_t)0xFFFF00
#define CYAN       (uint32_t)0x00FFFF
#define MAGENTA    (uint32_t)0xFF00FF
#define PURPLE     (uint32_t)0x400080
#define ORANGE     (uint32_t)0xFF3000
#define PINK       (uint32_t)0xFF1493

typedef enum {
	FX_OK		= 0,
	FX_ERROR	= 1
} FX_STATUS;

typedef enum {
FX_MODE_STATIC,
FX_MODE_WHITE_TO_COLOR,
FX_MODE_BLACK_TO_COLOR,
FX_MODE_BLINK,
FX_MODE_BLINK_RAINBOW,
FX_MODE_STROBE,
FX_MODE_STROBE_RAINBOW,
FX_MODE_BREATH,
FX_MODE_COLOR_WIPE,
FX_MODE_COLOR_WIPE_INV,
FX_MODE_COLOR_WIPE_REV,
FX_MODE_COLOR_WIPE_REV_INV,
FX_MODE_COLOR_WIPE_RANDOM,
FX_MODE_COLOR_SWEEP_RANDOM,
FX_MODE_RANDOM_COLOR,
FX_MODE_SINGLE_DYNAMIC,
FX_MODE_MULTI_DYNAMIC,
FX_MODE_RAINBOW,
FX_MODE_RAINBOW_CYCLE,
FX_MODE_FADE,
FX_MODE_SCAN,
FX_MODE_DUAL_SCAN,
FX_MODE_THEATER_CHASE,
FX_MODE_THEATER_CHASE_RAINBOW,
FX_MODE_RUNNING_LIGHTS,
FX_MODE_TWINKLE,
FX_MODE_TWINKLE_RANDOM,
FX_MODE_TWINKLE_FADE,
FX_MODE_TWINKLE_FADE_RANDOM,
FX_MODE_SPARKLE,
FX_MODE_FLASH_SPARKLE,
FX_MODE_HYPER_SPARKLE,
FX_MODE_MULTI_STROBE,
FX_MODE_CHASE_WHITE,
FX_MODE_CHASE_COLOR,
FX_MODE_CHASE_RANDOM,
FX_MODE_CHASE_RAINBOW,
FX_MODE_CHASE_FLASH,
FX_MODE_CHASE_FLASH_RANDOM,
FX_MODE_CHASE_RAINBOW_WHITE,
FX_MODE_CHASE_BLACKOUT,
FX_MODE_CHASE_BLACKOUT_RAINBOW,
FX_MODE_RUNNING_COLOR,
FX_MODE_RUNNING_RED_BLUE,
FX_MODE_RUNNING_RANDOM,
FX_MODE_LARSON_SCANNER,
FX_MODE_COMET,
FX_MODE_FIREWORKS,
FX_MODE_FIREWORKS_RANDOM,
FX_MODE_MERRY_CHRISTMAS,
FX_MODE_FIRE_FLICKER,
FX_MODE_FIRE_FLICKER_SOFT,
FX_MODE_FIRE_FLICKER_INTENSE,
FX_MODE_CIRCUS_COMBUSTUS,
FX_MODE_HALLOWEEN,
FX_MODE_BICOLOR_CHASE,
FX_MODE_TRICOLOR_CHASE,
FX_MODE_ICU,
} fx_mode;

FX_STATUS WS2812BFX_Init(uint16_t Segments);
FX_STATUS WS2812BFX_SegmentIncrease(void);
FX_STATUS WS2812BFX_SegmentDecrease(void);
uint8_t WS2812BFX_GetSegmentsQuantity(void);

void WS2812BFX_SysTickCallback(void);
void WS2812BFX_Callback(void);

FX_STATUS WS2812BFX_Start(uint16_t Segment);
FX_STATUS WS2812BFX_Stop(uint16_t Segment);
FX_STATUS WS2812BFX_IsRunning(uint16_t Segment, uint8_t *Running);

FX_STATUS WS2812BFX_SetMode(uint16_t Segment, fx_mode Mode);
FX_STATUS WS2812BFX_GetMode(uint16_t Segment, fx_mode *Mode);
FX_STATUS WS2812BFX_NextMode(uint16_t Segment);
FX_STATUS WS2812BFX_PrevMode(uint16_t Segment);
FX_STATUS WS2812BFX_SetReverse(uint16_t Segment, uint8_t Reverse);
FX_STATUS WS2812BFX_GetReverse(uint16_t Segment, uint8_t *Reverse);

FX_STATUS WS2812BFX_SetSegmentSize(uint16_t Segment, uint16_t Start, uint16_t Stop);
FX_STATUS WS2812BFX_GetSegmentSize(uint16_t Segment, uint16_t *Start, uint16_t *Stop);
FX_STATUS WS2812BFX_SegmentIncreaseEnd(uint16_t Segment);
FX_STATUS WS2812BFX_SegmentDecreaseEnd(uint16_t Segment);
FX_STATUS WS2812BFX_SegmentIncreaseStart(uint16_t Segment);
FX_STATUS WS2812BFX_SegmentDecreaseStart(uint16_t Segment);

FX_STATUS WS2812BFX_SetSpeed(uint16_t Segment, uint16_t Speed);
FX_STATUS WS2812BFX_GetSpeed(uint16_t Segment, uint16_t *Speed);
FX_STATUS WS2812BFX_IncreaseSpeed(uint16_t Segment, uint16_t Speed);
FX_STATUS WS2812BFX_DecreaseSpeed(uint16_t Segment, uint16_t Speed);

void WS2812BFX_SetColorStruct(uint8_t id, ws2812b_color c);
void WS2812BFX_SetColorRGB(uint8_t id, uint8_t r, uint8_t g, uint8_t b);
FX_STATUS WS2812BFX_GetColorRGB(uint8_t id, uint8_t *r, uint8_t *g, uint8_t *b);
void WS2812BFX_SetColorHSV(uint8_t id, uint16_t h, uint8_t s, uint8_t v);
void WS2812BFX_SetColor(uint8_t id, uint32_t c);

FX_STATUS WS2812BFX_SetAll(uint16_t Segment, uint32_t c);
FX_STATUS WS2812BFX_SetAllRGB(uint16_t Segment, uint8_t r, uint8_t g, uint8_t b);

void WS2812BFX_RGBtoHSV(uint8_t r, uint8_t g, uint8_t b, uint16_t *h, uint8_t *s, uint8_t *v);
void WS2812BFX_HSVtoRGB(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);

#endif /* WS2812B_FX_H_ */
