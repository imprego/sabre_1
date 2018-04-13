#ifndef __MEMS_H
#define __MEMS_H

#include <stdbool.h>

typedef enum
{
	GYROSCOPE,
	ACCELEROMETER,
	WHOAMI,
	READREG
} data_name;

void mems_init( void );
bool mems_is_accessable( void );
void get_mems_data( data_name name, uint8_t* recv_buf );
void print_fifo( bool new_print, char *new_format, bool new_full, bool new_endless );


#endif /* __MEMS_H */
