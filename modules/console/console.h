#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <stdbool.h>

void vConsoleInitProc( void* vParams );
void vConsoleWorkProc( void* vParams );
void send( char* data, unsigned short len, bool raw );

#endif /* __CONSOLE_H */
