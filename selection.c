#include "shared.h"

/* here we are using big numbers cuz we are basically turning the 2d rows and columns
 * into a 1d string and we want to insure that the number of for example row 1, column
 * 2 is still bigger than row 0, column 2 and we dont need to worry abt the rest of the
 * numbers cuz they are just increasing multipliers of 100K. 100k cuz the max terminal
 * width is 1K (i think) and 100K is just an overkill safe "buffer"
 */

bool is_selected(EditorConfig *E, int x, int y) {
    if (!E->selection_active) return false;

    long long current = (long long)y * 100000 + x;
    long long anchor = (long long)E->sel_anchor_y * 100000 + E->sel_anchor_x;
    long long cursor = (long long)E->cy * 100000 + E->cx;

    if (anchor < cursor) {
        return (current >= anchor && current < cursor);
    } else {
        return (current >= cursor && current < anchor);
    }
}

void get_selection_range(EditorConfig *E, int *sx, int *sy, int *ex, int *ey) {
    long long anchor = (long long)E->sel_anchor_y * 1000000 + E->sel_anchor_x;
    long long cursor = (long long)E->cy * 1000000 + E->cx;

    if (anchor <= cursor) {
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

        start_row->chars = realloc(start_row->chars, sx + end_part_len + 1);
        
        if (end_part_len > 0) {
            memcpy(&start_row->chars[sx], &end_row->chars[ex], end_part_len);
        }
        
        start_row->size = sx + end_part_len;
        start_row->chars[start_row->size] = '\0';

        int rows_to_remove = ey - sy;
        for (int i = 0; i < rows_to_remove; i++) {
            delete_row(E, sy + 1);
        }
    }

    E->cx = sx;
    E->cy = sy;
    E->selection_active = false;
    E->unsaved = true;
}
