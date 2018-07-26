# stm32-signal-generator
a signal generator program for the stm32 (STM32f469i Discovery board).

It uses the DAC and DMA to generate (stable) signals with up to 200 Khz frequency.

Can generate 2Mhz pwm signals.

Has a GUI with touchscreen.

features SINE, TRIANGLE, SAWTOOTH, PWM, and Draw mode which let's you draw arbitrary waveforms using the touchscreen.

the main user code is in the signal_generator.c file. 
(/BSP/SW4STM32/STM32469I_DISCO/Example/User/)

in the include folder you can find lookup maps for the signals, and pictures used for the menu. 

this code requires BSP library to work. 
