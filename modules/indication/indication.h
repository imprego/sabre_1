#ifndef __INDICATION_H
#define __INDICATION_H

typedef enum
{
	INDICATION_STATE_ON,
	INDICATION_STATE_OFF
} indication_state;

typedef enum
{
	INDICATION_LED_RED,
	INDICATION_LED_GREEN,
	INDICATION_LED_ALL
} indication_led;

void indication_init( void );
void set_indication( indication_led led, indication_state state );

#endif /* __INDICATION_H */
