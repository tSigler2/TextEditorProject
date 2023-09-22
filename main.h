#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>

struct editorConfig{
    int rows;
    int cols;
    int cx, cy;
    struct termios orig_termios;
};

void die(const char *s);

void disableRawMode();

void enableRawMode();

int getWindowSize(int *rows, int *cols);

struct abuf{
    char *b;
    int len;
};

void abAppend(struct abuf *ab, const char *s, int len);

void drawTildes(struct abuf *ab);

void refresh();

void MoveCursor(char k);

char readKeys();

void processKeys();

void getEditor();

int main();