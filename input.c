#include "shared.h"

void input(EditorConfig *E, char *c, bool *running){
    if (*c == CTRL_KEY('q')){
        //NOTE:
        // delete_empty_rows(E);
        *running = false;
    }
    else if (*c == CTRL_KEY('s')){
        delete_empty_rows(E);
        editor_save(E, c);
        redraw_screen(E);
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
                        if (seq[1] == '3') { // delete
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
                        if (E->cx < E->rows[E->cy].size) {
                            E->cx++;
                        } 
                        else if (E->cy < E->numrows - 1) {
                            E->cy++;
                            E->cx = 0;
                        }
                        break;

                    case 'D': // LEFT
                        if (E->cx > 0) {
                            E->cx--;
                        } 
                        else if (E->cy > 0) {
                            E->cy--;
                            E->cx = E->rows[E->cy].size;
                        }
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
        else if (E->cx == 0 && E->cy > 0){
            E->cy--;
            E->cx = E->rows[E->cy].size;
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


char *editor_prompt(EditorConfig *E, char *prompt, char *c) {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';

    while (1) {
        // Render the prompt at the very bottom line
        // Move to last line (E->height), clear it, and print
        printf("\x1b[%d;1H\x1b[K%s%s", E->height, prompt, buf);
        fflush(stdout);

        int ch = *c;

        if (ch == BACKSPACE || ch == 0x7f || ch == CTRL_KEY('h')) {
            if (buflen != 0) buf[--buflen] = '\0';
        } else if (ch == '\x1b') { // Escape to cancel
            free(buf);
            return NULL;
        } else if (ch == '\r') { // Enter to confirm
            if (buflen != 0) return buf;
        } else if (!iscntrl(ch) && ch < 128) {
            if (buflen + 1 >= bufsize) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = ch;
            buf[buflen] = '\0';
        }
    }
}
