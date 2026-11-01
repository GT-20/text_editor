#include "shared.h"

static bool pos_less(int ay, int ax, int by, int bx) {
    return ay < by || (ay == by && ax < bx);
}

bool is_selected(EditorConfig *E, int x, int y) {
    if (!E->selection_active) return false;

    if (pos_less(E->sel_anchor_y, E->sel_anchor_x, E->cy, E->cx)) {
        return !pos_less(y, x, E->sel_anchor_y, E->sel_anchor_x) &&
               pos_less(y, x, E->cy, E->cx);
    }

    return !pos_less(y, x, E->cy, E->cx) &&
           pos_less(y, x, E->sel_anchor_y, E->sel_anchor_x);
}

void get_selection_range(EditorConfig *E, int *sx, int *sy, int *ex, int *ey) {
    if (!pos_less(E->cy, E->cx, E->sel_anchor_y, E->sel_anchor_x)) {
        *sx = E->sel_anchor_x; *sy = E->sel_anchor_y;
        *ex = E->cx;           *ey = E->cy;
    } else {
        *sx = E->cx;           *sy = E->cy;
        *ex = E->sel_anchor_x; *ey = E->sel_anchor_y;
    }
}

void editor_delete_selection(EditorConfig *E) {
    if (!E->selection_active) return;

    int sx, sy, ex, ey;
    get_selection_range(E, &sx, &sy, &ex, &ey);

    if (sy == ey) {
        erow *row = &E->rows[sy];
        int bytes_to_delete = ex - sx;
        memmove(&row->chars[sx], &row->chars[ex], row->size - ex + 1);
        row->size -= bytes_to_delete;
    } 
    else {
        erow *start_row = &E->rows[sy];
        erow *end_row = &E->rows[ey];

        int end_part_len = end_row->size - ex;
        int new_cap = sx + end_part_len + 1;

        char *new_chars = realloc(start_row->chars, (size_t)new_cap);
        if (new_chars == NULL) {
            fprintf(stderr, "Out of memory!\n");
            return;
        }
        start_row->chars = new_chars;
        start_row->capacity = new_cap;
        
        if (end_part_len > 0) {
            memcpy(&start_row->chars[sx], &end_row->chars[ex], end_part_len);
        }
        
        start_row->size = sx + end_part_len;
        start_row->chars[start_row->size] = '\0';

        int rows_to_remove = ey - sy;
        for (int i = 0; i < rows_to_remove; i++) {
            delete_row(E, sy + 1);
        }

        editor_shrink_rows(E);
    }

    E->cx = sx;
    E->cy = sy;
    E->selection_active = false;
    E->unsaved = true;
}

char *editor_get_selection_text(EditorConfig *E, int *len_out) {
    if (!E->selection_active) {
        if (len_out) *len_out = 0;
        return NULL;
    }

    int sx, sy, ex, ey;
    get_selection_range(E, &sx, &sy, &ex, &ey);

    if (sy == ey) {
        if (ex <= sx) {
            if (len_out) *len_out = 0;
            return NULL;
        }
        int len = ex - sx;
        char *buf = malloc(len + 1);
        memcpy(buf, &E->rows[sy].chars[sx], len);
        buf[len] = '\0';
        if (len_out) *len_out = len;
        return buf;
    }

    int total = 0;
    erow *start_row = &E->rows[sy];
    erow *end_row = &E->rows[ey];
    total += (start_row->size - sx) + 1;
    for (int i = sy + 1; i < ey; i++) {
        total += E->rows[i].size + 1;
    }
    total += ex;

    char *buf = malloc(total + 1);
    char *p = buf;
    memcpy(p, &start_row->chars[sx], start_row->size - sx);
    p += (start_row->size - sx);
    *p++ = '\n';

    for (int i = sy + 1; i < ey; i++) {
        memcpy(p, E->rows[i].chars, E->rows[i].size);
        p += E->rows[i].size;
        *p++ = '\n';
    }

    if (ex > 0) {
        memcpy(p, end_row->chars, ex);
        p += ex;
    }
    *p = '\0';

    if (len_out) *len_out = p - buf;
    return buf;
}
