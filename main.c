#include "main.h"

/*Definitions*/

#define CTRL_KEY(k) ((k) & 0x1f)
#define STDVER "0.0.1"

enum eKeys{
    ARROW_LEFT = 'a',
    ARROW_RIGHT = 'd',
    ARROW_UP = 'w',
    ARROW_DOWN = 's'
};

/*Data*/

struct editorConfig E;

/*Terminal*/

void die(const char *s){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode(){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) die("tcsetattr");
}

void enableRawMode(){
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios r = E.orig_termios;

    r.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    r.c_oflag &= ~(OPOST);
    r.c_cflag |= (CS8);
    r.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    r.c_cc[VMIN] = 0;
    r.c_cc[VTIME] = 1;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &r) == -1) die("tcsetattr");
}

int getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        return -1;
    }
    else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;

        return 0;
    }
}

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len){
    char *new = realloc(ab->b, ab->len + len);

    if(new == NULL) return;

    memcpy(&new[ab->len], s, len);

    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab){
    free(ab->b);
}

/*Output*/

void drawTildes(struct abuf *ab){
    int i;

    for(i = 0; i < E.rows; i++){
        if (i == E.rows/3){
            char top[80];

            int wLen = snprintf(top, sizeof(top), "Standard Text Editor -- Version %s", STDVER);
            if(wLen > E.cols) wLen = E.cols;
            abAppend(ab, top, wLen);
            int p = (E.cols - wLen)/2;
            if(p){
                abAppend(ab, "~", 1);
                p--;
            }
        }
        else{
            abAppend(ab, "~", 1);
        }

        abAppend(ab, "\x1b[k", 3);
        if(i < E.rows - 1){
            abAppend(ab, "\r\n", 2);
        }
    }
}

void refresh(){
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 4);

    drawTildes(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[H", 3);
    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/*Input*/

void MoveCursor(char k){
    switch(k){
        case ARROW_LEFT:
            E.cx--;
            break;
        case ARROW_RIGHT:
            E.cx++;
            break;
        case ARROW_UP:
            E.cy--;
            break;
        case ARROW_DOWN:
            E.cy++;
            break;
    }
}

char readKeys(){
    int nr;
    char c;

    while((nr = read(STDIN_FILENO, &c, 1)) != 1){
        if(nr == -1 && errno != EAGAIN) die("read");
    }

    if(c == '\x1b'){
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if(seq[0] == '['){
            switch(seq[1]){
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }

        return '\x1b';
    }
    else{
        return c;
    }
}

void processKeys(){
    char c = readKeys();

    switch(c){
        case CTRL_KEY('q'):
            exit(0);
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            MoveCursor(c);
            break;
    }
}


/*Initiation of Program*/

void getEditor(){
    E.cx = 0;
    E.cy = 0;

    if(getWindowSize(&E.rows, &E.cols) == -1) die("WindowSize");
}

int main(){
    enableRawMode();
    getEditor();

    while(1){
        refresh();
        processKeys();
    }

    return 0;
}