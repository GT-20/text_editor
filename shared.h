#ifndef SHARED

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
    bool unsaved;
} EditorConfig;

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY
};

#ifdef DEBUG_EXISTS
    void editor_config_print(EditorConfig *E);
#endif

void enableRawMode();
void disableRawMode();
void clear_screen(void);
void editor_free(EditorConfig *E);
void editor_save(EditorConfig *E, int *c);
int editorReadKey();

void input(EditorConfig *E, int *c, bool *running);
char *editor_prompt(EditorConfig *E, char *prompt);

void delete_empty_rows(EditorConfig *E);
void delete_row(EditorConfig *E, int at);
void add_row(EditorConfig *E);
void break_into_newline(EditorConfig *E);
void redraw_screen(EditorConfig *E);
void move_cursor(EditorConfig *E);

#endif // !SHARED
