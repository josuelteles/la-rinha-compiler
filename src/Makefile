CC = gcc
CFLAGS = -g -I. -O3

SRC = rinha.c main.c
EXE = la-rinha

all: build

build:
	$(CC) $(CFLAGS) $(SRC) -o $(EXE)

clean:
	rm -f $(EXE)

.PHONY: all build
