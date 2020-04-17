CC=g++
CFLAGS=
LIBS=

all: bin/game.exe

bin/game.exe:
	$(CC) src/main.cc $(CFLAGS) $(LIBS) -o bin/game.exe
