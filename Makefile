all: build

.PHONY: all build test clean

build:
	$(MAKE) -C ./src
	$(MAKE) -C ./tests

test:
	./tests/la-rinha-tests

clean:
	$(MAKE) clean -C ./src
	$(MAKE) clean -C ./tests



