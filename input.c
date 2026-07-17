#include "shared.h"

void input(EditorConfig *E, int *c, bool *running){
    int key = *c;

    if (key == CTRL_KEY('q')){
        if (E->unsaved) {
            char *choice = editor_prompt(E, "Quit without saving? (y/N): ");
            if (choice != NULL && (choice[0] == 'y' || choice[0] == 'Y')) {
                *running = false;
            }
            if (choice) free(choice);
        } else *running = false;
        return; 
    }
    else if (key == CTRL_KEY('s')){
        // delete_empty_rows(E);
        E->unsaved = false;
        editor_save(E, c);
        redraw_screen(E);
    }

    switch (key) {
        case ARROW_UP:
            if (E->cy > 0) {
                E->cy--;
                if (E->cx > E->rows[E->cy].size) E->cx = E->rows[E->cy].size;
            }
            return;
        case ARROW_DOWN:
            if (E->cy < E->numrows - 1) {
                E->cy++;
                if (E->cx > E->rows[E->cy].size) E->cx = E->rows[E->cy].size;
            }
            return;
        case ARROW_RIGHT:
            if (E->cx < E->rows[E->cy].size) {
                E->cx++;
            } else if (E->cy < E->numrows - 1) {
                E->cy++;
                E->cx = 0;
            }
            return;
        case ARROW_LEFT:
            if (E->cx > 0) {
                E->cx--;
            } else if (E->cy > 0) {
                E->cy--;
                E->cx = E->rows[E->cy].size;
            }
            return;
        case DEL_KEY:
            erow *current_row = &E->rows[E->cy];

            if (E->cx < E->rows[E->cy].size && E->cx < current_row->size && current_row->size > 0) {
                memmove(&current_row->chars[E->cx], &current_row->chars[E->cx + 1], current_row->size - E->cx + 1);
                current_row->size--;
            }
            E->unsaved = true;
            return;
    }
    
    if (*c == '\r' || *c == '\n'){
        if (E->cx == E->rows[E->cy].size) {
            add_row(E);
        }
        else {
            break_into_newline(E);
        }
        E->unsaved = true;
    }

    else if (*c == BACKSPACE || *c== '\b'){
        erow *current_row = &E->rows[E->cy];

        if (E->cx > 0 && current_row->size > 0) {
            memmove(&current_row->chars[E->cx - 1], &current_row->chars[E->cx], current_row->size - E->cx + 1);
            current_row->size--;
            E->cx--;
        }
        else if (E->cx == 0 && E->cy > 0) {
            erow *prev_row = &E->rows[E->cy - 1];
            erow *current_row = &E->rows[E->cy];

            int target_cx = prev_row->size;

            int new_capacity = prev_row->size + current_row->size + 1;
            prev_row->chars = realloc(prev_row->chars, new_capacity);
            prev_row->capacity = new_capacity;

            memcpy(&prev_row->chars[prev_row->size], current_row->chars, current_row->size);
            prev_row->size += current_row->size;
            prev_row->chars[prev_row->size] = '\0';

            int row_to_delete = E->cy;
            E->cy--;
            E->cx = target_cx;

            delete_row(E, row_to_delete);
        }
        E->unsaved = true;
    }

    else{
        erow *current_row = &E->rows[E->cy];

        if(current_row->size+1 >= current_row->capacity){
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
}

char *editor_prompt(EditorConfig *E, char *prompt) {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';

    while (1) {
        printf("\x1b[%d;1H\x1b[K%s%s", E->height, prompt, buf);
        fflush(stdout);

        int ch = editorReadKey();

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
