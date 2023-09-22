CC=gcc
CFLAGS=-I.
DEPS = main.h
OBJ = main.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

texteditor: $(OBJ)
	$(CC) -o $@ $^

.PHONY: clean

clean:
	rm -f $(OBJ) 
	rm -f texteditor