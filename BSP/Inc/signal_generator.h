/*
 * dLock.h
 *
 *  Created on: Jul 4, 2018
 *      Author: TraIning
 */

#ifndef EXAMPLE_USER_DLOCK_H_
#define EXAMPLE_USER_DLOCK_H_




#include "anime.h"
#include "stm32f4xx_hal_dac.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dac_ex.h"


 void anime_draw_object(anime_object_t object,
                        uint16_t         pos_x,
                        uint16_t         pos_y,
                        uint8_t          transparency,
                        anime_object_t   buffer);
//typedef
//***************************************
#define MAIN_MENU 200
#define OPTIONS_MENU 250
#define GENERATE_MENU 100
#define DEBOUNCE_TIME 150
#define SCREEN_HEIGHT (uint16_t) 480
#define MAX_STEPS (uint16_t) 512
#define FRONT 0
#define BACK 1
static const uint32_t TIMER_BASE_FREQUENCY = 180000000UL ;

typedef enum MB {

	SINE , TRIANGLE , SAWTOOTH , SQUARE , OPTIONS , NO_TOUCH = 50 , CHANGE_SCREEN = 60 , ALTERNATE_FUNCTION = 70

} mainButtons;


typedef enum ON {

	OK = 10 , BACKSPACE ,
	FREQUENCY ,AMPLITUDE ,DC_OFFSET ,DUTY_CYCLE,

} numberButtons;




typedef struct recCoor{

	uint16_t xMin ;
	uint16_t xMax ;
	uint16_t yMin ;
	uint16_t yMax ;
} coordinates;

typedef struct signalSettings {

	uint32_t  frequency ;
	uint32_t  amplitude ;
	uint32_t  dcOffSet  ;
	uint32_t  dutyCycle ;
	uint32_t  wave 	;
	uint8_t   noise ;
} signalCharacteristics ;
//************************************
//vars and constants

extern uint32_t ts_status ;



//************************************
//function prototyping

void signal_generator_start();

static double  mp ( double value, double from1, double to1, double from2, double to2) {
   return (value - from1) / (to1 - from1) * (to2 - from2) + from2;
}
//************************************

#endif /* EXAMPLE_USER_DLOCK_H_ */
