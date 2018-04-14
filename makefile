CC = gcc
CFLAGS = -Wall
DEPS = 20162004.h
OBJ = 20162004.o shellCommands.o memoryCommands.o opcodeCommands.o generalFunctions.o asmCommands.o linkloadCommands.o
FILENAME = 20162004.out

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(FILENAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJ) $(FILENAME)
