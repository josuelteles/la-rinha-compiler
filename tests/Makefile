CC = gcc
CFLAGS = -g -I. -I../src -O3

SRC = ../src/rinha.c test.c
EXE = la-rinha-tests

all: build

build:
	$(CC) $(CFLAGS) $(SRC) -o $(EXE)

clean:
	rm -f $(EXE)

.PHONY: all build clean
