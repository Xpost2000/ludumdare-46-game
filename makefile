CC=g++
CFLAGS=-g -std=gnu++11
LIBS=-lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer

all: bin/treewatcher.out

bin/treewatcher.out: src/main.cpp
	$(CC) src/main.cpp $(CFLAGS) $(LIBS) -o bin/treewatcher.out
