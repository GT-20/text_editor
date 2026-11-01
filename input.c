#include "shared.h"
#include <stdio.h>
#include <stdlib.h>

void move_logic(EditorConfig *E, int key);
void handle_paste(EditorConfig *E);

void input(EditorConfig *E, int *c, bool *running){
    int key = *c;
    bool printable = isprint((unsigned char)key) != 0;

    bool replaces_selection = (key == BACKSPACE || key == '\b' || key == DEL_KEY ||
                               key == '\r' || key == '\n' || key == TAB ||
                               key == BRACKETED_PASTE || printable);

    if (E->selection_active && replaces_selection) {
        editor_delete_selection(E);
        if (key == BACKSPACE || key == '\b' || key == DEL_KEY) {
            return;
        }
    }

    if (key == CTRL_KEY('q')){
        if (E->unsaved) {
            char *choice = editor_prompt(E, "Quit without saving? (y/n): ");
            if (choice != NULL && (choice[0] == 'y' || choice[0] == 'Y')) {
                *running = false;
            }
            if (choice) free(choice);
        } else *running = false;
        return; 
    }
    
    if (key == CTRL_KEY('s')){
        if (editor_save(E)) {
            E->unsaved = false;
        }
        return;
    }

    if (key == CTRL_KEY('c')){
        if (E->selection_active) {
            editor_copy_selection(E);
            E->selection_active = false;
        }
        return;
    }

    switch (key) {
        case SHFT_UP:
        case SHFT_DOWN:
        case SHFT_LEFT:
        case SHFT_RIGHT:
            if (!E->selection_active) {
                E->selection_active = true;
                E->sel_anchor_x = E->cx;
                E->sel_anchor_y = E->cy;
            }
            move_logic(E, key);
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
        case CTRL_ARROW_UP:
        case CTRL_ARROW_DOWN:
        case CTRL_ARROW_LEFT:
        case CTRL_ARROW_RIGHT:
            E->selection_active = false;
            move_logic(E, key);
            break;

        case BRACKETED_PASTE:
            handle_paste(E);
            E->unsaved = true;
            break;

        case DEL_KEY:
            if (E->cy < E->numrows) {
                erow *current_row = &E->rows[E->cy];
                if (E->cx < current_row->size) {
                    int x = E->cx;
                    while (x < current_row->size && (current_row->chars[x] == ' ' || current_row->chars[x] == '\t')) x++;
                    while (x < current_row->size && !(current_row->chars[x] == ' ' || current_row->chars[x] == '\t')) x++;
                    int del_count = x - E->cx;
                    if (del_count == 0) del_count = 1;
                    memmove(&current_row->chars[E->cx], &current_row->chars[E->cx + del_count], current_row->size - (E->cx + del_count) + 1);
                    current_row->size -= del_count;
                    E->unsaved = true;
                } else if (E->cy < E->numrows - 1) {
                    erow *next_row = &E->rows[E->cy + 1];
                    erow *curr_row = &E->rows[E->cy];
                    int new_size = curr_row->size + next_row->size;
                    char *new_chars = realloc(curr_row->chars, (size_t)new_size + 1);
                    if (new_chars == NULL) break;
                    curr_row->chars = new_chars;
                    curr_row->capacity = new_size + 1;
                    memcpy(&curr_row->chars[curr_row->size], next_row->chars, next_row->size);
                    curr_row->size = new_size;
                    curr_row->chars[curr_row->size] = '\0';
                    delete_row(E, E->cy + 1);
                    E->unsaved = true;
                }
            }
            break;
        case BACKSPACE:
        case '\b':
            if (E->cx > 0) {
                erow *row = &E->rows[E->cy];
                if (key == CTRL_KEY('h')) {
                    int start = E->cx - 1;
                    int x = start;
                    while (x > 0 && (row->chars[x] == ' ' || row->chars[x] == '\t') && (row->chars[x - 1] == ' ' || row->chars[x - 1] == '\t')) x--;
                    while (x > 0 && (row->chars[x - 1] == ' ' || row->chars[x - 1] == '\t')) x--;
                    while (x > 0 && !(row->chars[x - 1] == ' ' || row->chars[x - 1] == '\t')) x--;
                    int del_count = start - x + 1;
                    memmove(&row->chars[x], &row->chars[E->cx], row->size - E->cx + 1);
                    row->size -= del_count;
                    E->cx = x;
                    E->unsaved = true;
                } else {
                    memmove(&row->chars[E->cx - 1], &row->chars[E->cx], row->size - E->cx + 1);
                    row->size--;
                    E->cx--;
                    E->unsaved = true;
                }
            } else if (E->cy > 0) {
                erow *prev_row = &E->rows[E->cy - 1];
                erow *curr_row = &E->rows[E->cy];
                int target_cx = prev_row->size;
                int new_size = prev_row->size + curr_row->size;

                char *new_chars = realloc(prev_row->chars, (size_t)new_size + 1);
                if (new_chars == NULL) break;
                prev_row->chars = new_chars;
                prev_row->capacity = new_size + 1;
                memcpy(&prev_row->chars[prev_row->size], curr_row->chars, curr_row->size);
                prev_row->size = new_size;
                prev_row->chars[prev_row->size] = '\0';
                E->cx = target_cx;
                int row_to_del = E->cy;
                E->cy--;
                delete_row(E, row_to_del);
                E->unsaved = true;
            }
            break;
        case TAB: {
            erow *current_row = &E->rows[E->cy];
            int pad = TAB_SIZE - (E->cx % TAB_SIZE);
            while (current_row->size + pad >= current_row->capacity) {
                current_row->capacity *= 2;
                current_row->chars = realloc(current_row->chars, current_row->capacity);
            }
            memmove(&current_row->chars[E->cx + pad], &current_row->chars[E->cx], current_row->size - E->cx + 1);
            for (int i = 0; i < pad; i++) current_row->chars[E->cx + i] = ' ';
            E->cx += pad;
            current_row->size += pad;
            current_row->chars[current_row->size] = '\0';
            E->unsaved = true;
            break;
        }
        case '\r':
        case '\n':
            if (E->cx == E->rows[E->cy].size) add_row(E);
            else break_into_newline(E);
            E->unsaved = true;
            break;
        default:
            if (printable) {
                erow *current_row = &E->rows[E->cy];
                if (current_row->size + 1 >= current_row->capacity) {
                    current_row->capacity *= 2;
                    current_row->chars = realloc(current_row->chars, current_row->capacity);
                }
                memmove(&current_row->chars[E->cx + 1], &current_row->chars[E->cx], current_row->size - E->cx + 1);
                current_row->chars[E->cx] = key;
                E->cx++;
                current_row->size++;
                current_row->chars[current_row->size] = '\0';
                E->unsaved = true;
            }
            break;
    }
}

void move_logic(EditorConfig *E, int key) {
    if (key == CTRL_ARROW_LEFT) {
        if (E->cy < E->numrows && E->cx > 0) {
            erow *row = &E->rows[E->cy];
            int x = E->cx - 1;
            while (x > 0 && !(row->chars[x] == ' ' || row->chars[x] == '\t') && (row->chars[x - 1] == ' ' || row->chars[x - 1] == '\t')) x--;
            while (x > 0 && (row->chars[x - 1] == ' ' || row->chars[x - 1] == '\t')) x--;
            while (x > 0 && !(row->chars[x - 1] == ' ' || row->chars[x - 1] == '\t')) x--;
            E->cx = x;
        } else if (E->cy > 0) {
            E->cy--;
            E->cx = E->rows[E->cy].size;
        }
        return;
    }
    if (key == CTRL_ARROW_RIGHT) {
        if (E->cy < E->numrows) {
            erow *row = &E->rows[E->cy];
            if (E->cx < row->size) {
                int x = E->cx;
                while (x < row->size && (row->chars[x] == ' ' || row->chars[x] == '\t')) x++;
                while (x < row->size && !(row->chars[x] == ' ' || row->chars[x] == '\t')) x++;
                E->cx = x;
            } else if (E->cy < E->numrows - 1) {
                E->cy++;
                E->cx = 0;
            }
        }
        return;
    }

    switch (key) {
        case ARROW_UP:
        case SHFT_UP:
            if (E->cy > 0) {
                E->cy--;
                if (E->cx > E->rows[E->cy].size) E->cx = E->rows[E->cy].size;
            }
            break;
        case ARROW_DOWN:
        case SHFT_DOWN:
            if (E->cy < E->numrows - 1) {
                E->cy++;
                if (E->cx > E->rows[E->cy].size) E->cx = E->rows[E->cy].size;
            }
            break;
        case ARROW_LEFT:
        case SHFT_LEFT:
            if (E->cx > 0) E->cx--;
            else if (E->cy > 0) { E->cy--; E->cx = E->rows[E->cy].size; }
            break;
        case ARROW_RIGHT:
        case SHFT_RIGHT:
            if (E->cx < E->rows[E->cy].size) E->cx++;
            else if (E->cy < E->numrows - 1) { E->cy++; E->cx = 0; }
            break;
    }
}

char *editor_prompt(EditorConfig *E, char *prompt) {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';

    while (1) {
        printf("\x1b[%d;1H\x1b[K%s%s", E->height, prompt, buf);
        fflush(stdout);

        int ch = editor_readKey();

        if (ch == BACKSPACE || ch == CTRL_KEY('h')) {
            if (buflen != 0) buf[--buflen] = '\0';
        } else if (ch == '\x1b') {
            free(buf);
            return NULL;
        } else if (ch == '\r') {
            if (buflen != 0){
                printf("\x1b[%d;1H\x1b[K%s", E->height, "                             ");
                fflush(stdout);
                return buf; 
            }
        } else if (isprint((unsigned char)ch) && ch < 128) {
            if (buflen + 1 >= bufsize) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = ch;
            buf[buflen] = '\0';
        }
    }
}

void editorInsertString(EditorConfig *E, char *s, int len) {
    bool last_was_cr = false;

    for (int i = 0; i < len; i++) {
        if (s[i] == '\r') {
            break_into_newline(E);
            last_was_cr = true;
            continue;
        }
        if (s[i] == '\n') {
            if (last_was_cr) {
                last_was_cr = false;
                continue;
            }
            break_into_newline(E);
            last_was_cr = false;
            continue;
        }

        last_was_cr = false;

        erow *row = &E->rows[E->cy];
        if (row->size + 1 >= row->capacity) {
            int new_cap = row->capacity > 0 ? row->capacity * 2 : 128;
            while (new_cap < row->size + 2) new_cap *= 2;

            char *new_chars = realloc(row->chars, (size_t)new_cap);
            if (new_chars == NULL) return;
            row->chars = new_chars;
            row->capacity = new_cap;
        }

        memmove(&row->chars[E->cx + 1], &row->chars[E->cx], row->size - E->cx + 1);
        row->chars[E->cx] = s[i];
        E->cx++;
        row->size++;
    }
}

static bool paste_buffer_append(char **buf, size_t *buflen, size_t *bufsize,
                                const char *data, size_t len) {
    if (*buflen + len > *bufsize) {
        size_t new_size = *bufsize > 0 ? *bufsize : 128;
        while (new_size < *buflen + len) new_size *= 2;

        char *tmp = realloc(*buf, new_size);
        if (tmp == NULL) return false;
        *buf = tmp;
        *bufsize = new_size;
    }

    memcpy(*buf + *buflen, data, len);
    *buflen += len;
    return true;
}

void handle_paste(EditorConfig *E) {
    static const char paste_end[] = "\x1b[201~";
    size_t bufsize = 12000;
    char *buf = malloc(bufsize);
    if (buf == NULL) return;
    size_t buflen = 0;
    char pending[6];
    int matched = 0;

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) break;

        if (matched == 0) {
            if (c == '\x1b') {
                pending[0] = c;
                matched = 1;
            } else if (!paste_buffer_append(&buf, &buflen, &bufsize, &c, 1)) {
                break;
            }
            continue;
        }

        if (c == paste_end[matched]) {
            pending[matched++] = c;
            if (matched == 6) {
                matched = 0;
                break;
            }
            continue;
        }

        if (!paste_buffer_append(&buf, &buflen, &bufsize, pending, (size_t)matched)) {
            break;
        }
        matched = 0;

        if (c == '\x1b') {
            pending[0] = c;
            matched = 1;
        } else if (!paste_buffer_append(&buf, &buflen, &bufsize, &c, 1)) {
            break;
        }
    }

    if (matched > 0) {
        paste_buffer_append(&buf, &buflen, &bufsize, pending, (size_t)matched);
    }

    editorInsertString(E, buf, (int)buflen);
    free(buf);
}
