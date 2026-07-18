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
