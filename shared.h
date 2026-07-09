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
#include <ctype.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define BACKSPACE 0x7f

typedef struct {
    char *chars;
    int size;
    int capacity;
} erow;

typedef struct {
    int cx, cy;
    int width, height;
    int start_row, start_col;
    int numrows;
    erow *rows;
    char *filename;
} EditorConfig;

typedef struct{
    char *buffer;
    size_t size;
    size_t gap_start;
    size_t gap_end;
} GapBuffer;

void erow_print(erow *row, int index);
void editor_config_print(EditorConfig *E);
void editor_free(EditorConfig *E);
void editor_save(EditorConfig *E, char *filename);

void input(EditorConfig *E, char *c, bool *running);

void delete_empty_rows(EditorConfig *E);
void add_row(EditorConfig *E);
void break_into_newline(EditorConfig *E);
void redraw_screen(EditorConfig *E);
void move_cursor(EditorConfig *E);

#endif // !SHARED
