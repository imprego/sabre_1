#include "power/power.h"
#include "stm32f4xx_hal.h"


#define PIN_POWER			GPIO_PIN_3
#define PORT_POWER		GPIOB


void power_init( void )
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitTypeDef gpio;
	//gpio.Alternate = ;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pin = PIN_POWER;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init( PORT_POWER, &gpio );
	HAL_GPIO_WritePin( PORT_POWER, PIN_POWER, GPIO_PIN_SET );
	return;
}

void set_power( power_state state )
{
	GPIO_PinState pin_state;
	if( state == POWER_ON )
		pin_state = GPIO_PIN_SET;
	else if( state == POWER_OFF )
		pin_state = GPIO_PIN_RESET;
	
	
	HAL_GPIO_WritePin( PORT_POWER, PIN_POWER, pin_state );
	return;
}
