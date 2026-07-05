#include "core.h"
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios);
}

void enableRawMode() {
    struct termios raw;

    if (!isatty(STDIN_FILENO)) exit(1);
    atexit(disableRawMode);
    if (tcgetattr(STDIN_FILENO,&orig_termios) == -1) exit(1);

    raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0; /* Return each byte, or zero for timeout. */
    raw.c_cc[VTIME] = 1; /* 100 ms timeout (unit is tens of second). */

    if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) < 0) exit(1);
}

void clear_screen(void) {
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
}
