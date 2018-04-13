#ifndef __TIMERS_H
#define __TIMERS_H

void timers_init( void );

void delay_ms( uint32_t delay );
void add_task( void (*fun_ptr)(void) );

#endif /* __TIMERS_H */
