#ifndef __POWER_H
#define __POWER_H

typedef enum
{
	POWER_ON,
	POWER_OFF
} power_state;

void power_init( void );
void set_power( power_state state );

#endif /* __POWER_H */
