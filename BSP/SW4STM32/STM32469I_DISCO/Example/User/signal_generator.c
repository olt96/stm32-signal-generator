/*
 * dLock.c
 *
 *  Created on: Jul 4, 2018
 *      Author: Oliver Trajkovski
 */



#include "main.h"
#include "anime.h"
#include "math.h"
#include "signal_generator.h"


#include "lookupTables/sin.h"
#include "lookupTables/triangle.h"
#include "lookupTables/sawTooth.h"
#include "uiGraphics/mainMenu.h"
#include "uiGraphics/optionsMenu.h"

//tables
 static const uint16_t *wavesTable [3][4] = {
  {sinTable64 , sinTable128 , sinTable256 , sinTable512 },
  {sawTable64 , sawTable128 , sawTable256 ,sawTable512 },
  {triangleTable64 , triangleTable128 , triangleTable256, triangleTable512} ,
 };
 static const char* optionStrings[4]={"FREQUENCY (Hz):" , "AMPLITUDE (V):"  , "DC OFFSET (V):" , "DUTY CYCLE (%):"};

 static anime_object_t anime_lcd_frame_buffer =
 {
   (uint32_t *)LCD_FB_START_ADDRESS,
   BPP_ARGB8888,   /* 0 = ARGB8888 format */
   800,
   SCREEN_HEIGHT,
   0x00000000, /* CLUT address */
   0,          /* CLUT size */
   0           /* animation pointer */
 };

 const coordinates mainMenuRegions [5]   = 	 {{0   , 180 , 0 , 0},
											  {180 , 350 , 0 , 0},
											  {350 , 520 , 0 , 0},
											  {520 , 690 , 0 , 0},
									 	  	  {690 , 800 , 0,  0}};


 const coordinates optionRegions   [16]  =	 {{430 , 620 , 400 , 480},
		 	 	 	 	 	 	 	 	 	  {250 , 430 , 140 , 230},
		 	 	 	 	 	 	 	 	 	  {430 , 620 , 140 , 230},
											  {620 , 800 , 140 , 230},
											  {250 , 430 , 230 , 310},
											  {430 , 620 , 230 , 310},
											  {620 , 800 , 230 , 310},
											  {250 , 430 , 310 , 400},
											  {430 , 620 , 310 , 400},
											  {620 , 800 , 310 , 400},
											  {250 , 430 , 400 , 480},
											  {620 , 800 , 400 , 480},
											  {0   , 250 , 140 , 230},
											  {0   , 250 , 230 , 310},
											  {0   , 250 , 310 , 400},
											  {0   , 250 , 400 , 480}};

//const and var
 static uint16_t generatedLookUp[MAX_STEPS] = {0};
 static uint8_t screenState = MAIN_MENU ;

 DAC_HandleTypeDef hdac;
 DMA_HandleTypeDef hdma_dac1;
 TIM_HandleTypeDef htim6;
 uint32_t ts_status = TS_OK ;
 static uint32_t *currentlyChanging  ;
 static signalCharacteristics settings ;
 //function declarations

static void draw_init ();
static void clk_init(void);
static void dac_init(void);
static void tim6_init(uint16_t psc);
static void signal_generator_run();
static void blow_text();
static void calculate_next_iteration(double *currentNumber , uint16_t *divider , uint8_t pressedButton);
static void handle_button (uint16_t pressedButton);
static void generate_wave ();
static uint16_t read_screen();
static void animate(uint8_t direction );
static void draw_grid();
static void settings_init();
static void screen_color (uint32_t color , uint8_t side );
static void calculate_step(uint16_t *steps , uint8_t *waveResolution);
static void write_to_wave(uint16_t x ,uint16_t y);
static void led_switch(uint8_t LED );
static void draw_number(double number);
static void draw_screen ();
static void draw_number_int(uint32_t  number);
static void pwm_init();
static void pwm (uint16_t psc);

void signal_generator_start(){

	  BSP_LED_Init(LED1);
	  BSP_LED_Init(LED2);
	  BSP_LED_Init(LED3);
	  BSP_LED_Init(LED4);
	  clk_init();
	  dac_init();
	  tim6_init(16);
	  settings_init();
	  draw_init();
	  pwm_init();
	  signal_generator_run();

}


static void signal_generator_run(){


	uint32_t  prevTime = HAL_GetTick();

	while (1) {
		if(HAL_GetTick() - prevTime > DEBOUNCE_TIME){

			    uint16_t  pressedButton = read_screen();
	  			if(pressedButton != NO_TOUCH){
			    handle_button (pressedButton);
	  		    prevTime = HAL_GetTick();

	  			}

	  	}

	}

}





static void calculate_step(uint16_t *steps , uint8_t *waveResolution){
uint32_t lowFRange = 10;
uint32_t highFRange = 30000;
for (uint32_t i = 0; i <= 3 ; i++ ){
	if(settings.frequency >= lowFRange && settings.frequency <= highFRange) break;
	*waveResolution = *waveResolution-1;
	lowFRange=highFRange ;
	highFRange*=2;
	*steps/=2;

}

}

static uint32_t colors [2] = {LCD_COLOR_RED , LCD_COLOR_GREEN};
static void handle_button (uint16_t pressedButton) {
	static uint16_t divider = 1;
	static double currentNumber = 0.0 ;
	uint16_t temp ;
	if(pressedButton == NO_TOUCH) return ;

	switch (screenState) {

	case MAIN_MENU :
		switch(pressedButton) {
		case ALTERNATE_FUNCTION :
			settings.noise = !settings.noise ;
			screen_color(colors[settings.noise] , BACK);
			screen_color(LCD_COLOR_BLACK , FRONT);
			BSP_LCD_DisplayStringAt(713 , 0 ,   (uint8_t*)"NOISE", LEFT_MODE);
			generate_wave();
			break;
		case SINE :
		case TRIANGLE :
		case SAWTOOTH :
			settings.wave = pressedButton ;
			generate_wave ();
			draw_grid();
			break;
		case SQUARE :
			settings.wave = pressedButton ;
			screenState = GENERATE_MENU ;
			draw_screen();
			break;
		case CHANGE_SCREEN :
			screenState = OPTIONS_MENU ;
			generate_wave();
			draw_screen();
			break;

		}

		break;

	case OPTIONS_MENU :
		switch(pressedButton){
	case FREQUENCY :

		led_switch(pressedButton);
		currentlyChanging =  &settings.frequency ;
		draw_number_int(*currentlyChanging);
		currentNumber = 0.0 ;

			break ;
	case AMPLITUDE :

		led_switch(pressedButton);
		currentlyChanging =  &settings.amplitude ;
		draw_number(mp(*currentlyChanging ,0 , 255 , 0.0 , 3.33));
		divider = 1;
		currentNumber = 0.0 ;
			break ;
	case DC_OFFSET :

		led_switch(pressedButton);
		currentlyChanging =  &settings.dcOffSet ;
		draw_number(mp(*currentlyChanging ,0 , 255 , 0.0 , 3.33));
		divider = 1;
		currentNumber = 0.0 ;
		 break ;

	case DUTY_CYCLE :

		led_switch(pressedButton);
		currentlyChanging =  &settings.dutyCycle ;
		draw_number_int(*currentlyChanging);
		currentNumber = 0.0 ;

			break ;

	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		calculate_next_iteration(&currentNumber , &divider , pressedButton);
			break;

	case CHANGE_SCREEN :
				screenState = MAIN_MENU;
				draw_screen();
			break;

	case BACKSPACE:


		if(currentlyChanging == &settings.amplitude ||currentlyChanging == &settings.dcOffSet ){
		currentNumber = 0.0;
		divider = 1 ;
		draw_number(currentNumber);
		}
		else{
			currentNumber = (uint32_t)currentNumber / 10 ;
			draw_number_int((uint32_t)round(currentNumber));
		}

		break ;

	case OK :
		BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
		BSP_LCD_DisplayStringAt(0 , 50 , (uint8_t*)"saved " , LEFT_MODE);
		BSP_LCD_DisplayStringAt(0 , 70 ,(uint8_t*) optionStrings[(currentlyChanging - &settings.frequency )] , LEFT_MODE);
		if(currentlyChanging == &settings.amplitude ||currentlyChanging == &settings.dcOffSet ){
		temp = mp(currentNumber , 0.0 , 3.33 , 0.0 , 256.0) ;
		if(temp  >255) temp = 255;
			*currentlyChanging = temp;
		}else{
			if(currentlyChanging == &settings.frequency && currentNumber < 10 ) currentNumber = 10 ;
			*currentlyChanging = currentNumber;
		}
		generate_wave();
			break ;


	}
		break;

		case GENERATE_MENU :
			if(pressedButton == CHANGE_SCREEN){
				screenState = MAIN_MENU ;
				generate_wave();
				animate(2);
				draw_grid();
			}


			break;


	}



}


static void calculate_next_iteration(double *currentNumber , uint16_t *divider , uint8_t pressedButton){

	if(currentlyChanging == &settings.amplitude ||currentlyChanging == &settings.dcOffSet ){

				*currentNumber+=(pressedButton/(double)*divider);
				if(*divider <=100)
				*divider*=10;
				*currentNumber = roundf(*currentNumber * 100) / 100;
				if(*currentNumber >= 3.33)
					*currentNumber = 3.33;
				draw_number(*currentNumber);

			}else{
			if((currentlyChanging == &settings.frequency && *currentNumber <= 20000 )
			|| (currentlyChanging == &settings.dutyCycle && *currentNumber <= 10 )){
			*currentNumber=(uint32_t)*currentNumber*10;
			*currentNumber += pressedButton;
			draw_number_int((uint32_t)round(*currentNumber));
			}
			}

}


static uint16_t  read_screen(){


	TS_StateTypeDef  TS_State = {0};
	ts_status = BSP_TS_GetState(&TS_State);

	uint16_t x ;
	uint16_t y ;

	if(TS_State.touchDetected)
	  {
		 x = TouchScreen_Get_Calibrated_X(TS_State.touchX[0]);
		 y = TouchScreen_Get_Calibrated_Y(TS_State.touchY[0]);
		 switch (screenState) {
		 case MAIN_MENU :
			 if(TS_State.touchDetected > 1 ) return ALTERNATE_FUNCTION ;
			 if(y < SCREEN_HEIGHT-100) return NO_TOUCH ;
			 for(int i = 0 ; i < 5 ; i++) {
				 if(x>mainMenuRegions[i].xMin && x < mainMenuRegions[i].xMax) {
					 if(i == OPTIONS) return CHANGE_SCREEN;
					 return i ;
				 }
			 }
			 break ;

		 case OPTIONS_MENU :
			 if(y  < 140) {

			 return CHANGE_SCREEN ;

			 }
			 for(int i = 0 ; i < 16 ; i++) {

				if( (x>optionRegions[i].xMin && x < optionRegions[i].xMax ) && (y>optionRegions[i].yMin && y < optionRegions[i].yMax ) ) {
								 return i ;

				}
			}

			 break ;

		 case GENERATE_MENU :
			 if(TS_State.touchDetected > 1 ){
				 return CHANGE_SCREEN ;
			 }
			 if( y  > 470)return NO_TOUCH;
			 write_to_wave(x , y);
			 return NO_TOUCH ;
			 break;

		 }


	  }

	return NO_TOUCH ;

}


//works but can cause hard faults , lots of bugs
static void write_to_wave(uint16_t x ,uint16_t y){

				uint16_t step  = MAX_STEPS;
				uint8_t waveRez = 3;

				calculate_step(&step , &waveRez);
				uint16_t t = mp(x , 1 , 790 , 0 , 64);
				if(t > 64) t = 64;
				uint8_t val = 255 - mp(y , 0 , 470 , 0 , 255);
				for(int i = -step/64 ; i <step/64 ;i ++ ){
				uint16_t temp = (t*(step/64))+i;
				if(temp < 0 || temp > MAX_STEPS) continue ;
				if(temp >490) {
					for(int i = temp;  i  < MAX_STEPS; i++ ) generatedLookUp[i] = val ;
					generatedLookUp[0] = val;
					return;
				}
				generatedLookUp[temp] = val ;

				}
				screen_color(LCD_COLOR_BLACK , FRONT);
				BSP_LCD_FillRect(x ,0 , x+20, SCREEN_HEIGHT );
				screen_color(LCD_COLOR_YELLOW , FRONT);
				BSP_LCD_DrawHLine(x ,y , 20 );
}

static void generate_wave (){





	 uint16_t steps = MAX_STEPS ;
	 uint8_t waveResolution = 3 ;
	 calculate_step(&steps , &waveResolution);
	 double  prescaler  =  TIMER_BASE_FREQUENCY  / (double)(settings.frequency*steps);

	 //DEBUG INFO
	 double scaleFactor =  mp (settings.amplitude , 0 , 255 , 0 , 1);
	 /*char  n[20] ;

	 BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	 sprintf(n , "debug info: %.2f  %d %d"  , scaleFactor   , steps , waveResolution);
	 BSP_LCD_DisplayStringAt(0 , 0 ,  n, LEFT_MODE);
	 sprintf(n , "debug info: %d %d %d %d"  , settings.dcOffSet  , settings.dutyCycle ,settings.frequency , settings.dutyCycle);
	 BSP_LCD_DisplayStringAt(0 , 30 ,  n, LEFT_MODE);*/
	 //DEBUG INFO
	 if(settings.wave != SQUARE)
	 for(int i = 0 ;  i < steps ;i ++){

		 uint16_t temp = ((wavesTable[settings.wave][waveResolution][i] -127)*scaleFactor)+127 + settings.dcOffSet;
		 if(temp > 255) temp = 255 ;
		 generatedLookUp[i] = temp ;// wavesTable[settings.wave][waveResolution][i] + settings.dcOffSet;

	 }


			tim6_init(prescaler);
			HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);
	  		HAL_DAC_Stop_DMA(&hdac,  DAC_CHANNEL_1);
	  		HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)generatedLookUp ,steps, DAC_ALIGN_8B_R);
	  		HAL_DAC_Start(&hdac,DAC_CHANNEL_1);


	  		settings.noise?
	  		HAL_DACEx_NoiseWaveGenerate(&hdac , DAC_CHANNEL_1 , DAC_LFSRUNMASK_BITS6_0 |DAC_LFSRUNMASK_BITS5_0)://TRUE
	  		(DAC1->CR &= ~DAC_CR_WAVE1_Msk);											//FALSE
	  		prescaler  =  TIMER_BASE_FREQUENCY  / (double)(settings.frequency*10);
	  					pwm (prescaler);


}




//****************************************
static void pwm (uint16_t psc) {

		  		  TIM_HandleTypeDef htim3;
		  		  TIM_OC_InitTypeDef sConfigOC;

		  		  htim3.Instance = TIM3;
		  		  htim3.Init.Prescaler = 0;
		  		  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
		  		  htim3.Init.Period = psc;
		  		  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
		  		  HAL_TIM_PWM_Init(&htim3) ;


		  		  sConfigOC.OCMode = TIM_OCMODE_PWM1;
		  		  uint16_t temp = 	psc*(settings.dutyCycle / 100.0);
		  		  if(settings.dutyCycle == 100 ) temp = psc ;
		  		  sConfigOC.Pulse = temp;
		  		  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
		  		  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
		  		  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) ;

		  		  HAL_TIM_PWM_Start(&htim3 , TIM_CHANNEL_1 );

}

/*initialization functions*/
static void pwm_init(){
					  __HAL_RCC_TIM3_CLK_ENABLE();

				      TIM_HandleTypeDef htim3;
				      TIM_MasterConfigTypeDef sMasterConfig;

			  		  htim3.Instance = TIM3;
			  		  htim3.Init.Prescaler = 0;
			  		  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
			  		  htim3.Init.Period = 10000;
			  		  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
			  		  HAL_TIM_PWM_Init(&htim3) ;


			  		  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
			  		  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
			  		  HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) ;

			  		  GPIO_InitTypeDef GPIO_InitStruct;
	 	 	 	 	  GPIO_InitStruct.Pin = GPIO_PIN_4;
					  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
					  GPIO_InitStruct.Pull = GPIO_NOPULL;
				      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
				      GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
					  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


}

static void settings_init(){

	settings.frequency = 1000 ;
	settings.amplitude = 255;
	settings.dutyCycle = 100 ;
	settings.wave = SQUARE ;
	settings.dcOffSet = 0 ;
	settings.noise = 0 ;
	currentlyChanging = &settings.frequency ;
	generate_wave();

}


static void dac_init(void)
{

  DAC_ChannelConfTypeDef sConfig;

    /**DAC Initialization
    */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {

  }

    /**DAC channel OUT1 config
    */
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {

  }

}


static void tim6_init(uint16_t psc)
{
if(psc < 10) psc = 10;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim6.Instance = TIM6;
  htim6.Init.Prescaler =0;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = psc-1;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {

  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {

  }
  HAL_TIM_Base_Start(&htim6);

}



/**
  * Enable DMA controller clock
  */
static void clk_init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  //HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  //HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  //DMA1_Stream5->CR &= ~(0x1U << DMA_SxCR_TCIE_Pos);

}

/** Pinout Configuration
*/

//wrapper za grda funkcija


/* GRAPHICS AND ANIMATING FUNCTIONS */




static void screen_color (uint32_t color , uint8_t side) {
		 side?
		 BSP_LCD_SetBackColor(color):
		 BSP_LCD_SetTextColor(color);

}

static void draw_grid(){
		char n [50];
		screen_color(LCD_COLOR_BLACK, FRONT );
		BSP_LCD_FillRect(0, 0, 800, SCREEN_HEIGHT-100 );
		BSP_LCD_DrawHLine(500 , 0 ,30);
		BSP_LCD_DrawHLine(500 , 1 ,30);
		screen_color(LCD_COLOR_YELLOW , FRONT);
		screen_color(LCD_COLOR_BLACK, BACK );

		//BSP_LCD_DrawVLine(BSP_LCD_GetXSize()/2 , 0 , BSP_LCD_GetYSize()-100);
		BSP_LCD_DrawHLine(0 , (BSP_LCD_GetYSize()-100 )/2 , BSP_LCD_GetXSize());
		uint16_t iter = 0 ;
		uint16_t steps  = MAX_STEPS;
		uint8_t temp ;
		calculate_step(&steps , &temp);
		//for(int k= -1 ; k <=1 ; k++)
		for(int i = 0 ; i < BSP_LCD_GetXSize() ; i++){
			BSP_LCD_FillRect(i , SCREEN_HEIGHT-100-mp(generatedLookUp[iter] , 0 , 255 , 50 , 330 )  , 2 , 2 );
			if(iter == steps) iter = 0 ;
			iter++;

		}

		    BSP_LCD_DisplayStringAt(0 , 0 ,  (uint8_t*)"frequency: ", LEFT_MODE);
		    settings.frequency < 1000 ?
		    sprintf(n , "%ld Hz" , settings.frequency ):
			sprintf(n , "%.2f KHz" , (double)settings.frequency/1000.0 );
			BSP_LCD_DisplayStringAt(160 , 0 ,  (uint8_t*)n , LEFT_MODE);
			sprintf(n , "%.2f V" , mp(settings.amplitude , 0 , 255 , 0 , 3.33));
		    BSP_LCD_DisplayStringAt(0 , 30 ,   (uint8_t*)"amplitude: ", LEFT_MODE);
			BSP_LCD_DisplayStringAt(160 , 30 , (uint8_t*)n, LEFT_MODE);
			BSP_LCD_DisplayStringAt(0 , 60 ,   (uint8_t*)"off set: ", LEFT_MODE);
			sprintf(n , "%.2f V" , mp(settings.dcOffSet , 0 , 255 , 0 , 3.33 ) );
			BSP_LCD_DisplayStringAt(160 , 60 , (uint8_t*)n    , LEFT_MODE);
			settings.frequency < 100000 ?
			sprintf(n , "%ld KHz" , settings.frequency/100 ):
			sprintf(n , "%.2f MHz" , settings.frequency/100000.0 );
			BSP_LCD_DisplayStringAt(0 , 350 ,  (uint8_t*) "PWM frequency:" , LEFT_MODE);
			BSP_LCD_DisplayStringAt(235 , 350 ,  (uint8_t*)n , LEFT_MODE);
			screen_color(LCD_COLOR_BLACK, FRONT );
			BSP_LCD_DrawHLine(500 , 0 ,30);
			BSP_LCD_DrawHLine(500 , 1 ,30);
		//for(int i = 0 ; i < )


}

static void draw_init (){


	  /* If calibration is not yet done, proceed with calibration */
	 BSP_LCD_Clear(LCD_COLOR_WHITE);
	 //BSP_LCD_DrawRect(0 , 0 , 800 , SCREEN_HEIGHT);
	 ts_status = Touchscreen_Calibration();


	 draw_grid();
	  anime_draw_object (mainMenu , 0 , SCREEN_HEIGHT-100 , 0xFF , anime_lcd_frame_buffer);




}

static void draw_number(double number){
	BSP_LCD_SetTextColor(0x18E3);
	BSP_LCD_FillRect(0, 0, 800, 140 );

	screen_color(LCD_COLOR_YELLOW , FRONT);
    screen_color(LCD_COLOR_BLACK, BACK);
	 char wut [50];
	sprintf(wut , "%.2f" ,number);
	BSP_LCD_DisplayStringAt(0 , 0 , (uint8_t*)wut , LEFT_MODE);
	blow_text();
	BSP_LCD_SetTextColor(0x18E3);
	BSP_LCD_FillRect(0, 0,  150, 18 );
}

static void draw_number_int(uint32_t  number){
	BSP_LCD_SetTextColor(0x18E3);
	BSP_LCD_FillRect(0, 0, 800, 140 );
	screen_color(LCD_COLOR_YELLOW , FRONT);
    screen_color(LCD_COLOR_BLACK, BACK);
	 char wut [50];
	sprintf(wut , "%lu" ,number);
	BSP_LCD_DisplayStringAt(0 , 0 , (uint8_t*)wut , LEFT_MODE);
	blow_text();
	BSP_LCD_SetTextColor(0x18E3);
	BSP_LCD_FillRect(0, 0,  150, 140 );
}

//go zgolemuva tekstot , ova moze da se naprai so pogolem font
//ama ova e grdo i brzo resenie
static void blow_text(){
	for(int i = 0 ; i < 150 ; i++){
	for(int j = 2 ; j <18; j++ ){
		uint32_t pix = BSP_LCD_ReadPixel(i , j );
		BSP_LCD_SetTextColor(pix);
		for(int k = 0 ; k < 7 ; k++)
		BSP_LCD_DrawHLine((i+70)*4 , (j*7)+k ,  4);

		}

	}


}

static void draw_screen (){


	//BSP_LCD_Clear(LCD_COLOR_WHITE);

	switch (screenState) {
	case MAIN_MENU :
		animate(MAIN_MENU);
		draw_grid();
		anime_draw_object (mainMenu , 0 , SCREEN_HEIGHT-100 , 0xFF , anime_lcd_frame_buffer);

		break ;

	case OPTIONS_MENU :
		 animate(OPTIONS_MENU);
		//  anime_draw_object (optionsMenu , 0 , 140 , 0xFF , anime_lcd_frame_buffer);
		  BSP_LCD_SetTextColor(0x18E3);
		  BSP_LCD_FillRect(0, 0, 800, 140 );



		break;
	case GENERATE_MENU :

		BSP_LCD_Clear(LCD_COLOR_BLACK);
		screen_color(LCD_COLOR_YELLOW , FRONT);
		screen_color(LCD_COLOR_BLACK , BACK);
		BSP_LCD_DisplayStringAt(0, 200 , (uint8_t*)"DRAW THE WAVEFORM YOU WANT TO SEE", CENTER_MODE);
		BSP_LCD_DisplayStringAt(0, 230 , (uint8_t*)"DOUBLE CLICK TO RETURN", CENTER_MODE);
		break;

	}


}

static void animate(uint8_t direction ){


	if(direction == 2){
		for(int i = 480 ; i >=380; i-=2)
				anime_draw_object (mainMenu , 0 , i , 0xFF , anime_lcd_frame_buffer);
		return;

	}
	if(direction == OPTIONS_MENU){
	for(int i = BSP_LCD_GetYSize() ; i >=140 ; i-=34)
	anime_draw_object (optionsMenu , 0 ,i ,0xFF , anime_lcd_frame_buffer);
	}else{
		for(int i = 140 ; i <=BSP_LCD_GetYSize() ; i+=34){
	    screen_color(LCD_COLOR_BLACK , FRONT);

		BSP_LCD_FillRect(0 , 0 , BSP_LCD_GetXSize()  , i);
		anime_draw_object (optionsMenu , 0 , i , 0xFF , anime_lcd_frame_buffer);
		}
		}

}


static void led_switch(uint8_t LED){
	BSP_LED_On(3- (LED-12));
 for (int i = 0 ; i <= 3 ; i++)
	 if(i!=(3- (LED-12))) BSP_LED_Off(i);

}




 void anime_draw_object(anime_object_t object,
                            uint16_t         pos_x,
                            uint16_t         pos_y,
                            uint8_t          transparency,
                            anime_object_t   buffer)
{
  uint32_t bytePerPixel = 0;

  if(buffer.bpp == BPP_ARGB8888)
  {
    bytePerPixel = 4;
  }
  else if(buffer.bpp == BPP_RGB888) /* RGB888 format */
  {
    bytePerPixel = 3;
  }
  else if ((buffer.bpp == BPP_RGB565)   ||
           (buffer.bpp == BPP_ARGB1555) ||
           (buffer.bpp == BPP_ARGB4444) ||
           (buffer.bpp == BPP_AL88))   /* RGB565 format */
  {
    bytePerPixel = 2;
  }
  else if ((buffer.bpp == BPP_L8)   ||
           (buffer.bpp == BPP_AL44) ||
           (buffer.bpp == BPP_A8))
  {
    bytePerPixel = 1;
  }

  /* Configure FG object */
  DMA2D->FGMAR = (uint32_t)(object.address);
  DMA2D->FGOR = 0x00000000; /* No offset in Src for FG object */

  /* If format of texture of 'object' contains an alpha information */
  if((object.bpp == BPP_ARGB8888)  ||
     (object.bpp == BPP_ARGB1555)  ||
     (object.bpp == BPP_ARGB4444)  ||
     (object.bpp == BPP_AL44)      ||
     (object.bpp == BPP_AL88)      ||
     (object.bpp == BPP_A8)        ||
     (object.bpp == BPP_A4))
  {
    /* Alpha Mode Choice */
    /* AM[1:0] = 2b10' => (0x2 << 16) : replace original alpha FG from texture by fixed alpha x original alpha texture */
    DMA2D->FGPFCCR = (transparency << 24) + (0x2 << 16) + object.bpp;
  }
  else
  {
    /* No Alpha in texture of 'object' */
    /* Alpha Mode Choice */
    /* AM[1:0] = 2b01' => (0x1 << 16) : replace original alpha FG by ALPHA[7:0] : ie 'transparency' parameter */
    DMA2D->FGPFCCR = (transparency << 24) + (0x1 << 16) + object.bpp;
  }

  DMA2D->NLR = (object.size_x << 16) + object.size_y;

  /* Configure BG object */
  /* Check if destination has direct color mode */
  DMA2D->BGMAR = (uint32_t)buffer.address + (bytePerPixel * ((pos_y * buffer.size_x) + pos_x));
  DMA2D->BGOR = buffer.size_x - object.size_x; /* Shall be same as output */
  /* ALPHA[7:0] is not used, AM[1:0] = 2b00' : no modification of the original alpha embedded in texture */
  DMA2D->BGPFCCR = buffer.bpp; /* CM[3:0] = buffer.bpp */
  DMA2D->OMAR = DMA2D->BGMAR;
  DMA2D->OOR = DMA2D->BGOR;
  DMA2D->OPFCCR = DMA2D->BGPFCCR;

  /* Clear flags */
  DMA2D->IFCR = 0x3F;
  /* Launch */
  DMA2D->CR = 0x00020001; /* Type of Transfer is M2M with blending and PFC + Start bit (0x01) */
  /* Wait till the copy is over */
  while (!DMA2D->ISR);
  DMA2D->IFCR = 0x02;
}
