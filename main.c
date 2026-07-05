#include "shared.h"
#include <stdio.h>

void row_insert_char(erow *row, int at, char c) {
    if (at < 0 || at > row->size) return;

    if (row->size + 2 > row->capacity) {
        row->capacity *= 2;
        row->chars = realloc(row->chars, row->capacity);
    }

    memmove(&row->chars[at + 1],
            &row->chars[at],
            row->size - at + 1);   // includes '\0'

    row->chars[at] = c;
    row->size++;
}

void delete_row(EditorConfig *E, int at) {
    if (at < 0 || at >= E->numrows) return;

    free(E->rows[at].chars);

    E->numrows--;

    if (E->numrows == 0) {
        E->rows = realloc(E->rows, 0);
        return;
    }

    erow *tmp = realloc(E->rows, sizeof(erow) * E->numrows);
    if (tmp) E->rows = tmp;
}

void delete_empty_rows(EditorConfig *E){
    if(E->rows[E->numrows-1].chars[0] == '\0'){
        for (int i = E->numrows - 1; E->rows[i].chars[0] == '\0'; i--){
            if (E->rows[i].size == 0) {
                delete_row(E, E->numrows - 1);
            }
        }
    }
}

void refresh_screen(EditorConfig *E) {
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);   // clear + home
    for (int i = 0; i < E->numrows; i++) {
        write(STDOUT_FILENO, E->rows[i].chars, E->rows[i].size);
        write(STDOUT_FILENO, "\r\n", 2);
    }
    // put cursor back where it belongs
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E->cy + 1, E->cx + 1);
    write(STDOUT_FILENO, buf, len);
}

void move_cursor(EditorConfig *E) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E->cy+1, E->cx+1);
    write(STDOUT_FILENO, buf, len);
}

void add_row(EditorConfig *E) {
    erow *temp = realloc(E->rows, sizeof(erow) * (E->numrows + 1));
    if (temp == NULL) {
        fprintf(stderr, "Failed to allocate additional memory for new line.\n");
        exit(1);
    }
    E->rows = temp;

    erow *new_row = &E->rows[E->numrows]; 
    new_row->size = 0;
    new_row->capacity = 128;
    new_row->chars = calloc(1, new_row->capacity);
    
    if (new_row->chars == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    
    new_row->chars[0] = '\0';

    putchar('\n');
    putchar('\r');

    E->numrows++;
    E->cy++;
    E->cx = 0;
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

    current_row->chars[E->cx] = '\0';
    current_row->size = E->cx;

    insert_row(E, E->cy + 1, right, right_len);
    free(right);

    E->cy++;
    E->cx = 0;
}

int main(int argc, char **argv){
    enableRawMode();
    clear_screen();

    EditorConfig E = {0};
    
    E.numrows = 1;
    E.rows = calloc(1, sizeof(erow) * E.numrows);
    E.rows[0].capacity = 128;
    E.rows[0].size = 0;
    E.rows[0].chars = calloc(1, E.rows[0].capacity);
    E.rows[0].chars[0] = '\0';

    char c = '\0';
    while (running) {
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == CTRL_Q){
                delete_empty_rows(&E);
                running = false;
            }
            else if (c == '\r' || c == '\n'){
                if (E.cx == E.rows[E.cy].size) {
                    add_row(&E);
                }
                else {
                    break_into_newline(&E);
                }
                refresh_screen(&E);
            }

            else if (c == '\x1b') {
                char seq[3];
                if (read(STDIN_FILENO, &seq[0], 1) && read(STDIN_FILENO, &seq[1], 1)) {
                    if (seq[0] == '[') {
                        if (seq[1] >= '0' && seq[1] <= '9') {
                            // It's a sequence like [3~ (Delete) or [5~ (PageUp)
                            if (read(STDIN_FILENO, &seq[2], 1) && seq[2] == '~') {
                                if (seq[1] == '3') {
                                    erow *current_row = &E.rows[E.cy];

                                    if (E.cx > 0 && E.cx < current_row->size && current_row->size > 0) {
                                        memmove(&current_row->chars[E.cx], &current_row->chars[E.cx + 1], current_row->size - E.cx + 1);
                                        current_row->size--;
                                        move_cursor(&E);
                                        refresh_screen(&E);
                                    }
                                }
                            }
                        }

                        switch (seq[1]) {
                            case 'A': // UP
                                if (E.cy > 0) {
                                    E.cy--; 
                                    if(E.rows[E.cy].size < E.rows[E.cy + 1].size) E.cx = E.rows[E.cy].size;
                                }
                                break;

                            case 'B': // DOWN
                                if (E.cy < E.numrows - 1) {
                                    E.cy++;
                                    if(E.rows[E.cy].size < E.rows[E.cy - 1].size) E.cx = E.rows[E.cy].size;
                                }
                                break;

                            case 'C': // RIGHT
                                if (E.cx < E.rows[E.cy].size) { E.cx++; }
                                break;

                            case 'D': // LEFT
                                if (E.cx > 0) { E.cx--; }
                                break;

                        }
                    }
                }
                move_cursor(&E);
            }

            else if (c == BACKSPACE || c== '\b'){
                erow *current_row = &E.rows[E.cy];

                if (E.cx > 0 && current_row->size > 0) {
                    memmove(&current_row->chars[E.cx - 1], &current_row->chars[E.cx], current_row->size - E.cx + 1);
                    current_row->size--;
                    E.cx--;
                    move_cursor(&E);
                    refresh_screen(&E);
                }
            }

            else{
                erow *current_row = &E.rows[E.cy];

                if(current_row->size+1 >= current_row->capacity){
                    current_row->capacity *= 2;
                    current_row->chars = realloc(current_row->chars, current_row->capacity);
                }

                memmove(&current_row->chars[E.cx + 1], &current_row->chars[E.cx], current_row->size - E.cx + 1);
                current_row->chars[E.cx] = c;
                if (write(STDOUT_FILENO, &c, 1)){
                    E.cx++;
                    current_row->size++;
                    current_row->chars[current_row->size] = '\0';
                    refresh_screen(&E);
                }
            }
        }
    }
    clear_screen();
    editor_config_print(&E);
    return 0;
    //TODO: free the allocated memory
}
