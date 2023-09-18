all: build

.PHONY: all build tests test clean

build:
	$(MAKE) -C ./src
	$(MAKE) -C ./tests

test:
	./tests/la-rinha-tests

docker:
	docker build -t la-rinha:latest .

# Clean rule
clean:
	$(MAKE) clean -C ./src
	$(MAKE) clean -C ./tests



