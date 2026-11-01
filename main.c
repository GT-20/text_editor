#include "shared.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool running = true;

static volatile sig_atomic_t winch_flag = 0;

static void handle_winch(int sig) {
    (void)sig;
    winch_flag = 1;
}

struct abuf {
    char *b;
    int len;
    int cap;
};

#define ABUF_INIT {NULL, 0, 0}

static void ab_append(struct abuf *ab, const char *s, int len) {
    if (ab->len + len > ab->cap) {
        int new_cap = ab->cap > 0 ? ab->cap : 256;
        while (new_cap < ab->len + len) new_cap *= 2;

        char *new = realloc(ab->b, (size_t)new_cap);
        if (new == NULL) return;
        ab->b = new;
        ab->cap = new_cap;
    }

    memcpy(ab->b + ab->len, s, (size_t)len);
    ab->len += len;
}

static void ab_free(struct abuf *ab) {
    free(ab->b);
}

int get_gutter_width(EditorConfig *E);

static void update_window_size(EditorConfig *E) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        E->width = ws.ws_col;
        E->height = ws.ws_row;
    } else if (E->width <= 0 || E->height <= 0) {
        E->width = 80;
        E->height = 24;
    }
}

void delete_row(EditorConfig *E, int at) {
    if (at < 0 || at >= E->numrows) return;

    free(E->rows[at].chars);

    if (at < E->numrows - 1) {
        memmove(&E->rows[at], &E->rows[at + 1], sizeof(erow) * (E->numrows - at - 1));
    }

    E->numrows--;
}

void editor_shrink_rows(EditorConfig *E) {
    if (E->numrows <= 0 || E->rows_capacity <= E->numrows) return;

    erow *tmp = realloc(E->rows, sizeof(erow) * (size_t)E->numrows);
    if (tmp != NULL) {
        E->rows = tmp;
        E->rows_capacity = E->numrows;
    }
}

void editor_scroll(EditorConfig *E) {
    int draw_height = E->height - 2;
    int gutter_w = get_gutter_width(E);
    int draw_width = E->width - gutter_w;

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
    update_window_size(E);
    editor_scroll(E);

    struct abuf ab = ABUF_INIT;
    ab_append(&ab, "\x1b[?25l", 6);
    ab_append(&ab, "\x1b[H", 3);

    int draw_height = E->height - 2;
    int gutter_w = get_gutter_width(E);
    int draw_width = E->width - gutter_w;

    for (int i = 0; i < draw_height; i++) {
        int file_row_idx = i + E->start_row;
        ab_append(&ab, "\x1b[K", 3);

        if (file_row_idx < E->numrows) {
            char gutter[16];

            int g_len = snprintf(gutter, sizeof(gutter), "%*d ", gutter_w - 1, file_row_idx + 1);
            ab_append(&ab, "\x1b[90m", 5); //-gray 
            ab_append(&ab, gutter, g_len);
            ab_append(&ab, "\x1b[0m", 4); //-reset

            erow *row = &E->rows[file_row_idx];
            if (row->size > E->start_col) {
                int visible_len = row->size - E->start_col;
                if (visible_len > draw_width) visible_len = draw_width;

                for (int j = 0; j < visible_len; j++) {
                    int file_col = j + E->start_col;
                    if (is_selected(E, file_col, file_row_idx)) {
                        ab_append(&ab, "\x1b[7m", 4);
                        ab_append(&ab, &row->chars[file_col], 1);
                        ab_append(&ab, "\x1b[27m", 5);
                    } else {
                        ab_append(&ab, &row->chars[file_col], 1);
                    }
                }
            }
        }
        ab_append(&ab, "\r\n", 2);
    }

    char buf[32];
    int screen_y = (E->cy - E->start_row) + 1;
    int screen_x = (E->cx - E->start_col) + gutter_w + 1;
    if (screen_y < 1) screen_y = 1;
    if (screen_x < 1) screen_x = 1;
    int clen = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", screen_y, screen_x);
    ab_append(&ab, buf, clen);

    ab_append(&ab, "\x1b[?25h", 6); //-show cursor
    write(STDOUT_FILENO, ab.b, ab.len);
    ab_free(&ab);
    fflush(stdout);
}

static void ensure_row_capacity(EditorConfig *E, int needed) {
    if (needed <= E->rows_capacity) return;

    int new_cap = E->rows_capacity > 0 ? E->rows_capacity : 1;
    while (new_cap < needed) new_cap *= 2;

    erow *tmp = realloc(E->rows, sizeof(erow) * (size_t)new_cap);
    if (tmp == NULL) {
        fprintf(stderr, "Failed to allocate additional memory for new line.\n");
        exit(1);
    }

    E->rows = tmp;
    E->rows_capacity = new_cap;
}

void add_row(EditorConfig *E) {
    if (E->numrows == 0) {
        insert_row(E, 0, "", 0);
        E->cy = 0;
        E->cx = 0;
        return;
    }

    insert_row(E, E->cy + 1, "", 0);
    E->cy++;
    E->cx = 0;
}

void insert_row(EditorConfig *E, int at, const char *s, size_t len) {
    if (at < 0 || at > E->numrows) return;

    ensure_row_capacity(E, E->numrows + 1);

    char *chars = malloc(len + 1);
    if (chars == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    memcpy(chars, s, len);
    chars[len] = '\0';

    if (at < E->numrows) {
        memmove(&E->rows[at + 1], &E->rows[at], sizeof(erow) * (E->numrows - at));
    }

    E->rows[at].size = (int)len;
    E->rows[at].capacity = (int)len + 1;
    E->rows[at].chars = chars;

    E->numrows++;
}

void break_into_newline(EditorConfig *E){
    erow *current_row = &E->rows[E->cy];

    int right_len = current_row->size - E->cx;
    char *right = malloc((size_t)right_len + 1);
    if (right == NULL) {
        fprintf(stderr, "Out of memory!\n");
        return;
    }
    memcpy(right, &current_row->chars[E->cx], right_len);
    right[right_len] = '\0';

    current_row->chars[E->cx] = '\0'; current_row->size = E->cx;

    insert_row(E, E->cy + 1, right, right_len);
    free(right);

    E->cy++;
    E->cx = 0;
}

void open_file(EditorConfig *E, char *filename) {
    free(E->filename);
    E->filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return;
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    if (E->numrows == 1 && E->rows[0].size == 0) {
        free(E->rows[0].chars);
        E->rows[0].chars = NULL;
        E->rows[0].capacity = 0;
        E->numrows = 0;
    }

    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || 
                               line[linelen - 1] == '\r')) {
            linelen--;
        }
        
        insert_row(E, E->numrows, line, linelen);
    }

    free(line);
    fclose(fp);

    if (E->numrows == 0) {
        insert_row(E, 0, "", 0);
    }
}

int get_gutter_width(EditorConfig *E) {
    int lines = E->numrows;
    int width = 0;
    while (lines > 0) {
        lines /= 10;
        width++;
    }
    if (width < 2) width = 1;
    return width + 1;
}

int main(int argc, char **argv){
    enableRawMode();
    clear_screen();

    EditorConfig E = {0};
    
    E.numrows = 1;
    E.rows_capacity = 1;
    E.rows = calloc(1, sizeof(erow));
    if (E.rows == NULL) {
        fprintf(stderr, "Out of memory!\n");
        return 1;
    }
    E.rows[0].capacity = 128;
    E.rows[0].size = 0;
    E.rows[0].chars = calloc(1, E.rows[0].capacity);
    if (E.rows[0].chars == NULL) {
        fprintf(stderr, "Out of memory!\n");
        free(E.rows);
        return 1;
    }
    E.rows[0].chars[0] = '\0';
    E.start_row = 0;
    E.start_col = 0;
    E.unsaved = false;

    update_window_size(&E);
    signal(SIGWINCH, handle_winch);

    if (argc >= 2) {
        open_file(&E, argv[1]);
    }

    redraw_screen(&E);

    while (running) {
        if (winch_flag) {
            winch_flag = 0;
            update_window_size(&E);
            redraw_screen(&E);
        }

        int c = editor_readKey();
        input(&E, &c, &running);
        redraw_screen(&E);
    }

    clear_screen();

    #ifdef DEBUG_EXISTS
        editor_config_print(&E);
    #endif

    editor_free(&E);
    return 0;
}
