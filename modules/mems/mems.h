#ifndef __MEMS_H
#define __MEMS_H

void mems_init( void );

uint8_t* who_am_i( void );
uint8_t* self_test_config( void );
uint8_t* read_self_test( void );
uint8_t* read_gyro( void );
uint8_t* read_acc( void );


#endif /* __MEMS_H */
