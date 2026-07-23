#include "shared.h"

void move_logic(EditorConfig *E, int key);
void handle_paste(EditorConfig *E);

void input(EditorConfig *E, int *c, bool *running){
    int key = *c;

    bool trigger_sel_del= (key == BACKSPACE || key == '\b' || key == DEL_KEY || isprint(key) || key == '\r');

    if (E->selection_active && trigger_sel_del) {
        editor_delete_selection(E);
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
        editor_save(E, c);
        E->unsaved = false;
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
                    memmove(&current_row->chars[E->cx], &current_row->chars[E->cx + 1], current_row->size - E->cx);
                    current_row->size--;
                    E->unsaved = true;
                }
            }
            break;
        case BACKSPACE:
        case '\b':
            if (E->cx > 0) {
                erow *row = &E->rows[E->cy];
                memmove(&row->chars[E->cx - 1], &row->chars[E->cx], row->size - E->cx + 1);
                row->size--;
                E->cx--;
                E->unsaved = true;
            } else if (E->cy > 0) {
                erow *prev_row = &E->rows[E->cy - 1];
                erow *curr_row = &E->rows[E->cy];
                int target_cx = prev_row->size;

                prev_row->chars = realloc(prev_row->chars, prev_row->size + curr_row->size + 1);
                memcpy(&prev_row->chars[prev_row->size], curr_row->chars, curr_row->size);
                prev_row->size += curr_row->size;
                prev_row->chars[prev_row->size] = '\0';
                E->cx = target_cx;
                int row_to_del = E->cy;
                E->cy--;
                delete_row(E, row_to_del);
                E->unsaved = true;
            }
            break;
        case '\r':
        case '\n':
            if (E->cx == E->rows[E->cy].size) add_row(E);
            else break_into_newline(E);
            E->unsaved = true;
            break;
        default:
            if (isprint(key)) {
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

        if (ch == BACKSPACE || ch == 0x7f || ch == CTRL_KEY('h')) {
            if (buflen != 0) buf[--buflen] = '\0';
        } else if (ch == '\x1b') {
            free(buf);
            return NULL;
        } else if (ch == '\r') {
            if (buflen != 0) return buf;
        } else if (isprint(ch) && ch < 128) {
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
    for (int i = 0; i < len; i++) {
        if (s[i] == '\r' || s[i] == '\n') {
            break_into_newline(E);
        } else {
            // Direct insertion into the row
            erow *row = &E->rows[E->cy];
            if (row->size + 1 >= row->capacity) {
                row->capacity = row->size + 2; // Temporary small growth
                row->chars = realloc(row->chars, row->capacity);
            }
            memmove(&row->chars[E->cx + 1], &row->chars[E->cx], row->size - E->cx + 1);
            row->chars[E->cx] = s[i];
            E->cx++;
            row->size++;
        }
    }
}

void handle_paste(EditorConfig *E) {
    size_t bufsize = 12000;
    char *buf = malloc(bufsize);
    size_t buflen = 0;

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) break;

        // Check for end of paste sequence: \x1b [ 2 0 1 ~
        if (c == '\x1b') {
            char seq[6];
            if (read(STDIN_FILENO, &seq, 5) == 5) {
                if (seq[0] == '[' && seq[1] == '2' && seq[2] == '0' && seq[3] == '1' && seq[4] == '~') {
                    break; // End of paste
                }
            }
        }

        // Add character to buffer
        buf[buflen++] = c;
        if (buflen >= bufsize - 1) {
            bufsize *= 2;
            buf = realloc(buf, bufsize);
        }
    }

    editorInsertString(E, buf, buflen);
    free(buf);
}
