CC=g++
CFLAGS=-g -std=gnu++11
LIBS=-lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer

all: bin/game.exe

bin/game.exe: src/main.cpp
	$(CC) src/main.cpp $(CFLAGS) $(LIBS) -o bin/game.exe
