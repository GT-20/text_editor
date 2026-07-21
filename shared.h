#ifndef SHARED_H

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
    int sel_anchor_x, sel_anchor_y;
    bool selection_active;
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
    SHFT_UP,
    SHFT_DOWN,
    SHFT_LEFT,
    SHFT_RIGHT,
    BRACKETED_PASTE,
    DEL_KEY,
};

#ifdef DEBUG_EXISTS
    void editor_config_print(EditorConfig *E);
#endif

// core.c
void enableRawMode();
void disableRawMode();
void clear_screen(void);
void editor_free(EditorConfig *E);
void editor_save(EditorConfig *E, int *c);
int editor_readKey();

//input.c
void input(EditorConfig *E, int *c, bool *running);
char *editor_prompt(EditorConfig *E, char *prompt);

//selection.c
bool is_selected(EditorConfig *E, int x, int y);

//main.c
void delete_empty_rows(EditorConfig *E);
void delete_row(EditorConfig *E, int at);
void add_row(EditorConfig *E);
void break_into_newline(EditorConfig *E);
void redraw_screen(EditorConfig *E);
void update_cursor(EditorConfig *E);

#endif // !SHARED_H
