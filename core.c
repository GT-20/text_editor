#include "shared.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios orig_termios;

static void handle_fatal_signal(int sig) {
    disableRawMode();
    signal(sig, SIG_DFL);
    raise(sig);
}

void disableRawMode(void) {
    fflush(stdout);
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios);
        write(STDOUT_FILENO, "\x1b[?2004l", 8);
}

void enableRawMode(void) {
    struct termios raw;

    if (!isatty(STDIN_FILENO)) exit(1);
    if (tcgetattr(STDIN_FILENO,&orig_termios) == -1) exit(1);
    atexit(disableRawMode);
    signal(SIGINT, handle_fatal_signal);
    signal(SIGTERM, handle_fatal_signal);
    signal(SIGHUP, handle_fatal_signal);
    signal(SIGQUIT, handle_fatal_signal);
    signal(SIGSEGV, handle_fatal_signal);
    signal(SIGABRT, handle_fatal_signal);

    raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) < 0) exit(1);
    write(STDOUT_FILENO, "\x1b[?2004h", 8);
}

void clear_screen(void) {
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
}

int editor_readKey(void) {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1) exit(1);
    }

    if (c == '\x1b') {
        char seq[8];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] == '2' && read(STDIN_FILENO, &seq[2], 2) == 2) {
                 if (seq[2] == '0' && seq[3] == '0' && read(STDIN_FILENO, &seq[4], 1) == 1) {
                     if (seq[4] == '~') return BRACKETED_PASTE;
                 }
            }

            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';

                if (seq[1] == '1' && seq[2] == ';') {
                    if (read(STDIN_FILENO, &seq[3], 1) != 1) return '\x1b';
                    if (read(STDIN_FILENO, &seq[4], 1) != 1) return '\x1b';
                    if (seq[3] == '2') {
                        switch (seq[4]) {
                            case 'A': return SHFT_UP;
                            case 'B': return SHFT_DOWN;
                            case 'C': return SHFT_RIGHT;
                            case 'D': return SHFT_LEFT;
                        }
                    }
                    if (seq[3] == '5') {
                        switch (seq[4]) {
                            case 'A': return CTRL_ARROW_UP;
                            case 'B': return CTRL_ARROW_DOWN;
                            case 'C': return CTRL_ARROW_RIGHT;
                            case 'D': return CTRL_ARROW_LEFT;
                        }
                    }
                }

                if (seq[2] == '~') {
                    if (seq[1] == '3') return DEL_KEY;
                }
                if (seq[1] == '3' && seq[2] == ';' && read(STDIN_FILENO, &seq[3], 1) == 1 && seq[3] == '5' && read(STDIN_FILENO, &seq[4], 1) == 1 && seq[4] == '~') {
                    return DEL_KEY;
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                }
            }
        }
        return '\x1b';
    }
    return c;
}

void editor_free(EditorConfig *E) {
    if (E->filename) {
        free(E->filename);
        E->filename = NULL;
    }

    if (E->rows == NULL) return;

    for (int i = 0; i < E->numrows; i++) {
        if (E->rows[i].chars != NULL) {
            free(E->rows[i].chars);
        }
    }

    free(E->rows);
    E->rows = NULL;
    E->numrows = 0;
    E->rows_capacity = 0;
}

char *editor_rows_to_string(EditorConfig *E, int *buflen) {
    int total_len = 0;
    int j;

    for (j = 0; j < E->numrows; j++) {
        total_len += E->rows[j].size;
    }
    if (E->numrows > 1) {
        total_len += E->numrows - 1;
    }
    *buflen = total_len;

    char *buf = malloc(total_len > 0 ? total_len : 1);
    if (buf == NULL) return NULL;

    char *p = buf;

    for (j = 0; j < E->numrows; j++) {
        if (j > 0) *p++ = '\n';
        memcpy(p, E->rows[j].chars, E->rows[j].size);
        p += E->rows[j].size;
    }

    return buf;
}

bool editor_save(EditorConfig *E) {
    bool prompted = false;

    if (E->filename == NULL) {
        E->filename = editor_prompt(E, "Save as: ");
        if (E->filename == NULL) {
            return false;
        }
        prompted = true;
    }

    int len = 0;
    char *buf = editor_rows_to_string(E, &len);
    if (buf == NULL) {
        if (prompted) {
            free(E->filename);
            E->filename = NULL;
        }
        printf("\x1b[%d;1H\x1b[KSave failed! Out of memory.", E->height);
        fflush(stdout);
        sleep(2);
        printf("\x1b[%d;1H\x1b[K", E->height);
        return false;
    }

    char tmp_name[256];
    int name_len = snprintf(tmp_name, sizeof(tmp_name), "%s.tmp", E->filename);
    bool saved = false;

    if (name_len > 0 && (size_t)name_len < sizeof(tmp_name)) {
        FILE *fp = fopen(tmp_name, "w");
        if (fp != NULL) {
            bool written = fwrite(buf, 1, (size_t)len, fp) == (size_t)len;
            if (fclose(fp) != 0) written = false;

            if (written && rename(tmp_name, E->filename) == 0) {
                saved = true;
            } else {
                unlink(tmp_name);
            }
        }
    }

    free(buf);

    if (saved) {
        printf("\x1b[%d;1H\x1b[K%d bytes written to disk", E->height, len);
        fflush(stdout);
        sleep(1);
        return true;
    }

    if (prompted) {
        free(E->filename);
        E->filename = NULL;
    }

    printf("\x1b[%d;1H\x1b[KSave failed! I/O Error.", E->height);
    fflush(stdout);
    sleep(2);
    printf("\x1b[%d;1H\x1b[K", E->height);
    return false;
}

static bool copy_via_command(const char *cmd, const char *text, int len) {
    FILE *f = popen(cmd, "w");
    if (f == NULL) return false;

    bool written = fwrite(text, 1, (size_t)len, f) == (size_t)len;
    int status = pclose(f);
    return written && status == 0;
}

void editor_copy_selection(EditorConfig *E) {
    int len = 0;
    char *text = editor_get_selection_text(E, &len);
    if (text == NULL || len == 0) {
        if (text) free(text);
        return;
    }

    if (copy_via_command("wl-copy 2>/dev/null", text, len) ||
        copy_via_command("xclip -selection clipboard 2>/dev/null", text, len) ||
        copy_via_command("xsel --clipboard --input 2>/dev/null", text, len)) {
        free(text);
        return;
    }

    free(text);
}
