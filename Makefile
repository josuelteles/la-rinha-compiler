all: build

.PHONY: all build clean

build:
	$(MAKE) -C ./src

test:
	./tests/la-rinha-tests

clean:
	$(MAKE) clean -C ./src



