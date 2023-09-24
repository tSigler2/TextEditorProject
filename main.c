#include "main.h"

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
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12)) return -1;
        return cursorPosition(rows, cols);
    }
    else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;

        return 0;
    }
}
/*Row Operations*/

void addEditorRow(char *s, size_t len){
    E.r = realloc(E.r, sizeof(erow) * (E.numRows + 1));
    int at = E.numRows;

    E.r[at].size = len;
    E.r[at].cs = malloc(len+1);
    memcpy(E.r[at].cs, s, len);
    E.r[at].cs = '\0';
    E.numRows = 1;
}

/*File I/O*/
void openEditor(char *filename){
    FILE *fp = fopen(filename, "r");
    if(!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while((linelen = getline(&line, &linecap, fp)) != -1){
        while(linelen > 0 && (line[linelen-1] == '\n' || line[linelen-1] == '\r')){
            linelen--;
        }

        addEditorRow(line, linelen);
    }
    free(line);
    fclose(fp);
}

int cursorPosition(int *rows, int *cols){
    char buf[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while(i < sizeof(buf)-1){
        if(read(STDIN_FILENO, &buf[i], 1) != -1) break;
        if(buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if(buf[0] != '\x1b' || buf[1] != '[') return -1;
    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
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
        if(i >= E.numRows){
            if(E.numRows == 0 && i == E.rows/3){
                char heading[80];
                int headinglen = snprintf(heading, sizeof(heading), "STANDARD EDITOR -- version %s", STDVER);
                if(headinglen > E.cols) headinglen = E.cols;
                int padding = (E.cols - headinglen)/2;
                if(padding){
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while(padding--) abAppend(ab, " ", 1);
                abAppend(ab, heading, headinglen);
            }
        } 
        else{
            int len = E.r[i].size;
            if(len > E.cols) len = E.cols;
            abAppend(ab, E.r[i].cs, 1);
        }

        abAppend(ab, "\x1b[K", 3);
        if(i < E.rows-1){
            abAppend(ab, "\r\n", 2);
        }
    }
}

void refresh(){
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    drawTildes(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy+1, E.cx+1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[H", 3);
    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/*Input*/

void MoveCursor(int k){
    switch(k){
        case ARROW_LEFT:
            if(E.cx != 0){
                E.cx--;
            }
            break;
        case ARROW_RIGHT:
            if(E.cx != E.cols-1){
                E.cx++;
            }
            break;
        case ARROW_UP:
            if(E.cy != 0){
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if(E.cy != E.rows-1){
                E.cy++;
            }
            break;
    }
}

int readKeys(){
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
            if(seq[1] >= '0' && seq[1] <= '9'){
                if(read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if(seq[2] == '~'){
                    switch(seq[1]){
                        case '1': return HOME;
                        case '3': return DELETE;
                        case '4': return END;
                        case '5': return PAGEUP;
                        case '6': return PAGEDOWN;
                        case '7': return HOME;
                        case '8': return END;
                    }
                }
            }
            else{
                switch(seq[1]){
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME;
                    case 'F': return END;
                }
            }
        }
        else if(seq[0] == '0'){
                switch(seq[1]){
                    case 'H': return HOME;
                    case 'F': return END;
                }
            }

        return '\x1b';
    }
    else{
        return c;
    }
}

void processKeys(){
    int c = readKeys();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 4);
            exit(0);
            break;

        case HOME:
            E.cx = 0;
            break;
        case END:
            E.cx = E.cols-1;
            break;

        case PAGEUP:
        case PAGEDOWN:
            {
                int t = E.rows;
                while(t--) MoveCursor(c == PAGEUP ? ARROW_UP : ARROW_DOWN);
            }
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
    E.numRows = 0;
    E.r = NULL;

    if(getWindowSize(&E.rows, &E.cols) == -1) die("WindowSize");
}

int main(int argc, char *argv[]){
    enableRawMode();
    getEditor();
    if(argc >= 2){
        openEditor(argv[1]);
    }

    while(1){
        refresh();
        processKeys();
    }

    return 0;
}