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

#define BRIGHTNESS_MIN 		0
#define DEFAULT_BRIGHTNESS 	50
#define BRIGHTNESS_MAX 		255

#define MODE_COUNT 		62
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

void WS2812BFX_SysTickCallback(void);
void WS2812BFX_Callback(void);
void WS2812BFX_Start(void);
void WS2812BFX_Stop(void);

void WS2812BFX_SetMode(fx_mode mode);
void WS2812BFX_NextMode(void);
void WS2812BFX_PrevMode(void);

void WS2812BFX_SetSpeed(uint16_t Speed);
void WS2812BFX_IncreaseSpeed(uint16_t Speed);
void WS2812BFX_DecreaseSpeed(uint16_t Speed);

void WS2812BFX_SetColorStruct(uint8_t id, ws2812b_color c);
void WS2812BFX_SetColorRGB(uint8_t id, uint8_t r, uint8_t g, uint8_t b);
void WS2812BFX_SetColorHSV(uint8_t id, uint16_t h, uint8_t s, uint8_t v);
void WS2812BFX_SetColor(uint8_t id, uint32_t c);
void WS2812BFX_SetColorAll(uint32_t c);
void WS2812BFX_SetColorAllRGB(uint8_t r, uint8_t g, uint8_t b);

void WS2812BFX_RGBtoHSV(uint8_t r, uint8_t g, uint8_t b, uint16_t *h, uint8_t *s, uint8_t *v);
void WS2812BFX_HSVtoRGB(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);

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
#endif /* WS2812B_FX_H_ */
