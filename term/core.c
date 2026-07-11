#include "core.h"
#include "../shared.h"
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios orig_termios;

void disableRawMode() {
    fflush(stdout);
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios);
}

void enableRawMode() {
    struct termios raw;

    if (!isatty(STDIN_FILENO)) exit(1);
    atexit(disableRawMode);
    if (tcgetattr(STDIN_FILENO,&orig_termios) == -1) exit(1);

    raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) < 0) exit(1);
}

void clear_screen(void) {
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
}

void editor_free(EditorConfig *E) {
    if (E->rows == NULL) return;

    for (int i = 0; i < E->numrows; i++) {
        if (E->rows[i].chars != NULL) {
            free(E->rows[i].chars);
        }
    }

    if (E->filename) free(E->filename);
    free(E->rows);
    E->rows = NULL;
    E->numrows = 0;
}

char *editor_rows_to_string(EditorConfig *E, int *buflen) {
    int total_len = 0;
    int j;

    // calculate final string length
    for (j = 0; j < E->numrows; j++) {
        total_len += E->rows[j].size + 1;
    }
    *buflen = total_len;

    char *buf = malloc(total_len);
    char *p = buf;

    // insert \n at the end of each row
    for (j = 0; j < E->numrows; j++) {
        memcpy(p, E->rows[j].chars, E->rows[j].size);
        p += E->rows[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

void editor_save(EditorConfig *E, char *c) {
    if (E->filename == NULL) {
        E->filename = editor_prompt(E, "Save as: ", c);
        if (E->filename == NULL) {
            // User pressed ESC
            return;
        }
    }

    int len;
    char *buf = editor_rows_to_string(E, &len);

    // Create a path for the temporary file
    char tmp_name[256];
    snprintf(tmp_name, sizeof(tmp_name), "%s.tmp", E->filename);

    FILE *fp = fopen(tmp_name, "w");
    if (fp != NULL) {
        if (fwrite(buf, 1, len, fp) == len) {
            fclose(fp);
            // Move tmp file to original filename
            rename(tmp_name, E->filename);
            free(buf);
            
            // Optional: Print a status message
            printf("\x1b[%d;1H\x1b[K%d bytes written to disk", E->height, len);
            fflush(stdout);
            sleep(1); 
            return;
        }
        fclose(fp);
    }

    free(buf);
    // Error handling
    printf("\x1b[%d;1H\x1b[KSave failed! I/O Error.", E->height);
    fflush(stdout);
    sleep(2);
}
