#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>

/*Definitions*/

#define CTRL_KEY(k) ((k) & 0x1f)
#define STDVER "0.0.1"

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

enum eKeys{
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DELETE,
    HOME,
    END,
    PAGEUP,
    PAGEDOWN
};

typedef struct eRow{
    int size;
    char *cs;
} erow;

struct editorConfig{
    int rows;
    int cols;
    int cx, cy;
    erow *r;
    int numRows;
    struct termios orig_termios;
};

void die(const char *s);

void disableRawMode();

void enableRawMode();

int getWindowSize(int *rows, int *cols);

void addEditorRow(char *s, size_t len);

void addRow(char *s, size_t len);

void openEditor(char *filename);

int cursorPosition(int *rows, int *cols);

struct abuf{
    char *b;
    int len;
};

void abAppend(struct abuf *ab, const char *s, int len);

void abFree(struct abuf *ab);

void drawTildes(struct abuf *ab);

void refresh();

void MoveCursor(int k);

int readKeys();

void processKeys();

void getEditor();

int main(int argc, char *argv[]);