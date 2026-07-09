#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

bool running = true;

void move_cursor(EditorConfig *E);

void delete_row(EditorConfig *E, int at) {
    if (at < 0 || at >= E->numrows) return;

    free(E->rows[at].chars);

    E->numrows--;

    if (E->numrows == 0) {
        E->rows = realloc(E->rows, 0);
        perror("Number of rows reached 0, not possible, thus bug in program.");
        return;
    }

    erow *tmp = realloc(E->rows, sizeof(erow) * E->numrows);
    if (tmp) E->rows = tmp;
}

void delete_empty_rows(EditorConfig *E){
    // delete all the empty rows starting from the end of the file
    while (E->numrows > 1 && E->rows[E->numrows - 1].size == 0) {
        delete_row(E, E->numrows - 1);
    }
}

void editor_scroll(EditorConfig *E) {
    int draw_height = E->height - 2; // drawable area
    int draw_width = E->width;

    if (E->cy < E->start_row) {
        E->start_row = E->cy;
    }
    if (E->cy >= E->start_row + draw_height) {
        E->start_row = E->cy - draw_height + 1;
    }

    if (E->cx < E->start_col) {
        E->start_col = E->cx;
    }
    if (E->cx >= E->start_col + draw_width) {
        E->start_col = E->cx - draw_width + 1;
    }
}

void redraw_screen(EditorConfig *E) {
    editor_scroll(E);

    write(STDOUT_FILENO, "\x1b[?25l", 6);
    write(STDOUT_FILENO, "\x1b[H", 3);

    int draw_height = E->height - 2;

    for (int i = 0; i < draw_height; i++) {
        int file_idx = i + E->start_row;
        write(STDOUT_FILENO, "\x1b[K", 3);

        if (file_idx < E->numrows) {
            int len = E->rows[file_idx].size;
            
            // Calculate how much of this row is visible
            if (len > E->start_col) {
                int visible_len = len - E->start_col;
                
                // Clamp visible length to the screen width
                if (visible_len > E->width) visible_len = E->width;
                
                // Write starting from the start_col offset
                write(STDOUT_FILENO, &E->rows[file_idx].chars[E->start_col], visible_len);
            }
        } 
        write(STDOUT_FILENO, "\r\n", 2);
    }
    
    move_cursor(E);
    write(STDOUT_FILENO, "\x1b[?25h", 6);
    fflush(stdout);
}

void move_cursor(EditorConfig *E) {
    char buf[32];
    // screen_y = Logical Row - Scroll Offset + 1 (for 1-indexing)
    int screen_y = (E->cy - E->start_row) + 1;
    int screen_x = (E->cx - E->start_col) + 1;

    int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", screen_y, screen_x);
    write(STDOUT_FILENO, buf, len);
}

void add_row(EditorConfig *E) {
    erow *tmp = realloc(E->rows, sizeof(erow) * (E->numrows + 1));
    if (tmp == NULL) {
        fprintf(stderr, "Failed to allocate additional memory for new line.\n");
        exit(1);
    }
    E->rows = tmp;

    erow *new_row = &E->rows[E->numrows]; 
    new_row->size = 0;
    new_row->capacity = 128;
    new_row->chars = calloc(1, new_row->capacity);
    
    if (new_row->chars == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    
    new_row->chars[0] = '\0';

    write(STDOUT_FILENO, "\n\r", 2);

    E->numrows++;
    E->cy++;
    E->cx = 0;
    fflush(stdout);
}

void insert_row(EditorConfig *E, int at, const char *s, size_t len) {
    if (at < 0 || at > E->numrows) return;

    E->rows = realloc(E->rows, sizeof(erow) * (E->numrows + 1));
    memmove(&E->rows[at + 1], &E->rows[at], sizeof(erow) * (E->numrows - at));

    E->rows[at].chars = malloc(len + 1);
    memcpy(E->rows[at].chars, s, len);
    E->rows[at].chars[len] = '\0';
    E->rows[at].size = len;
    E->rows[at].capacity = len + 1;

    E->numrows++;
}

void break_into_newline(EditorConfig *E){
    erow *current_row = &E->rows[E->cy];

    int right_len = current_row->size - E->cx;
    char *right = malloc(right_len + 1);
    memcpy(right, &current_row->chars[E->cx], right_len);
    right[right_len] = '\0';

    current_row->chars[E->cx] = '\0'; current_row->size = E->cx;

    insert_row(E, E->cy + 1, right, right_len);
    free(right);

    E->cy++;
    E->cx = 0;
    fflush(stdout);
}

int main(int argc, char **argv){
    struct winsize ws;

    enableRawMode();
    clear_screen();

    EditorConfig E = {0};
    
    E.numrows = 1;
    E.rows = calloc(1, sizeof(erow) * E.numrows);
    E.rows[0].capacity = 128;
    E.rows[0].size = 0;
    E.rows[0].chars = calloc(1, E.rows[0].capacity);
    E.rows[0].chars[0] = '\0';
    E.start_row = 0;
    E.start_col = 0;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    E.width = ws.ws_col;
    E.height = ws.ws_row;

    if (argc > 1) {
        E.filename = strdup(argv[1]);
    } else {
        E.filename = NULL;
    }

    redraw_screen(&E);

    char c = '\0';
    while (running) {
        if (read(STDIN_FILENO, &c, 1) == 1) {
            input(&E, &c, &running);
        }
    }
    clear_screen();
    editor_config_print(&E);

    editor_free(&E);
    return 0;
}
