#include "shared.h"

void input(EditorConfig *E, char *c, bool *running){
    if (*c == CTRL_KEY('q')){
        delete_empty_rows(E);
        *running = false;
    }
    else if (*c == CTRL_KEY('s')){
        delete_empty_rows(E);
        editor_save(E, "test.txt");
        fflush(stdout);
    }

    else if (*c == '\r' || *c == '\n'){
        if (E->cx == E->rows[E->cy].size) {
            add_row(E);
        }
        else {
            break_into_newline(E);
        }
    }

    else if (*c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) && read(STDIN_FILENO, &seq[1], 1)) {
            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    // It's a sequence like [3~ (Delete) or [5~ (PageUp)
                    if (read(STDIN_FILENO, &seq[2], 1) && seq[2] == '~') {
                        if (seq[1] == '3') {
                            erow *current_row = &E->rows[E->cy];

                            if (E->cx < E->rows[E->cy].size && E->cx < current_row->size && current_row->size > 0) {
                                memmove(&current_row->chars[E->cx], &current_row->chars[E->cx + 1], current_row->size - E->cx + 1);
                                current_row->size--;
                            }
                        }
                    }
                }

                switch (seq[1]) {
                    case 'A': // UP
                        if (E->cy > 0) {
                            E->cy--; 
                            if (E->cx > E->rows[E->cy].size) E->cx = E->rows[E->cy].size;
                        }
                        break;
                    case 'B': // DOWN
                        if (E->cy < E->numrows - 1) {
                            E->cy++;
                            if (E->cx > E->rows[E->cy].size) E->cx = E->rows[E->cy].size;
                        }
                        break;

                    case 'C': // RIGHT
                        if (E->cx < E->rows[E->cy].size) E->cx++;
                        break;

                    case 'D': // LEFT
                        if (E->cx > 0) E->cx--;
                        break;
                }
            }
        }
    }

    else if (*c == BACKSPACE || *c== '\b'){
        erow *current_row = &E->rows[E->cy];

        if (E->cx > 0 && current_row->size > 0) {
            memmove(&current_row->chars[E->cx - 1], &current_row->chars[E->cx], current_row->size - E->cx + 1);
            current_row->size--;
            E->cx--;
        }
    }

    else{
        erow *current_row = &E->rows[E->cy];

        if(current_row->size+1 >= current_row->capacity){
            current_row->capacity *= 2;
            current_row->chars = realloc(current_row->chars, current_row->capacity);
        }

        memmove(&current_row->chars[E->cx + 1], &current_row->chars[E->cx], current_row->size - E->cx + 1);
        current_row->chars[E->cx] = *c;
        if (write(STDOUT_FILENO, c, 1)){
            E->cx++;
            current_row->size++;
            current_row->chars[current_row->size] = '\0';
        }
    }
    move_cursor(E);
    redraw_screen(E);
}
