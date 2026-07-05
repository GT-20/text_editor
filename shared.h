#ifndef SHARED

#include "term/core.h"

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <termios.h>

#include <string.h>
#include <sys/ioctl.h>

#define CTRL_Q 0x11
#define BACKSPACE 0x7f

typedef struct {
    char *chars;
    int size;
    int capacity;
} erow;

typedef struct {
    int cx, cy;
    int rowoff;
    int coloff;
    int numrows;
    erow *rows;
} EditorConfig;

typedef struct{
    char *buffer;
    size_t size;
    size_t gap_start;
    size_t gap_end;
} GapBuffer;

static bool running = true;

void erow_print(erow *row, int index);
void editor_config_print(EditorConfig *E);

#endif // !SHARED
