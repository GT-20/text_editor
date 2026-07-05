#include "shared.h"

void erow_print(erow *row, int index) {
    fprintf(stderr, "Row %d: size=%d, text=\"%s\"\r\n", 
            index, row->size, row->chars ? row->chars : "NULL");
}

void editor_config_print(EditorConfig *E) {
    fprintf(stderr, "--- Editor Config ---\r\n");
    fprintf(stderr, "Cursor: cx=%d, cy=%d\r\n", E->cx, E->cy);
    fprintf(stderr, "Offsets: rowoff=%d, coloff=%d\r\n", E->rowoff, E->coloff);
    fprintf(stderr, "Num Rows: %d\r\n", E->numrows);
    
    for (int i = 0; i < E->numrows; i++) {
        erow_print(&E->rows[i], i);
    }
    fprintf(stderr, "--------------------\r\n");
}

