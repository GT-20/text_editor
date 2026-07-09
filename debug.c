#include "shared.h"
#include <stdio.h>

void erow_print(erow *row, int index) {
    fprintf(stderr, "Row %d: size=%d, text=\"%s\"\r\n", 
            index, row->size, row->chars ? row->chars : "NULL");
}

void editor_config_print(EditorConfig *E) {
    fprintf(stderr, "--- Editor Config ---\r\n");
    fprintf(stderr, "Cursor: cx=%d, cy=%d\r\n", E->cx, E->cy);
    fprintf(stderr, "Num Rows: %d\r\n", E->numrows);
    fprintf(stderr, "Width: %d\r\n", E->width);
    fprintf(stderr, "Height: %d\r\n", E->height);
    fprintf(stderr, "Starting row: %d\r\n", E->start_row);
    fprintf(stderr, "relative cursor position: %d\r\n", E->cy-E->start_row);
    for (int i = 0; i < E->numrows; i++) {
        erow_print(&E->rows[i], i);
    }
    fprintf(stderr, "--------------------\r\n");
}
