#ifndef __SHOCK_H
#define __SHOCK_H

#include <stdint.h>

void shock_init( void );
void flush_shock_buf( void );
uint16_t get_shock_data( void );
uint16_t** get_shock_buf( void );

#endif /* __SHOCK_H */
