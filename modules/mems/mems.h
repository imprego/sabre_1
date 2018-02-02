#ifndef __MEMS_H
#define __MEMS_H


typedef enum
{
	GYROSCOPE,
	ACCELEROMETER,
	WHOAMI
} data_name;

void mems_init( void );
void get_mems_data( data_name name, uint8_t* recv_buf );


#endif /* __MEMS_H */
