#ifndef __TIMERS_H
#define __TIMERS_H

void timers_init( void );

void delay_ms( uint32_t delay );
void add_task( void (*pf)(void), unsigned int execute_rate );
uint32_t get_global_time( void );
void start_timer( void );

#endif /* __TIMERS_H */
