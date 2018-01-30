#ifndef __MEMS_H
#define __MEMS_H


typedef enum
{
	GYROSCOPE,
	ACCELEROMETER,
	WHOAMI
} data_name;

void mems_init( void );

uint8_t* who_am_i( void );
uint8_t* self_test_config( void );
uint8_t* read_self_test( void );
uint8_t* read_gyro( void );
uint8_t* read_acc( void );

uint8_t* get_data( data_name name );


#endif /* __MEMS_H */
