#ifndef CORE

#include <stddef.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

void enableRawMode();
void disableRawMode();
void clear_screen();

#endif // !CORE
