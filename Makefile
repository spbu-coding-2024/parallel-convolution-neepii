.PHONY: all test debug bear clean

CC:=gcc
CFLAGS:= -Werror -Wall -O3 -fopenmp
TEST_LDFLAGS:= -lcunit

SRC:=$(wildcard src/*.c)
TEST_SRC:=$(wildcard test/*.c)
OBJ:=$(patsubst src/%.c,build/%.o,$(SRC))
TEST_OBJ:=$(patsubst test/%.c,build/test/%.o,$(TEST_SRC))

EXEC_NAME:=conv
TEST_EXEC_NAME:=test_conv

all: build/$(EXEC_NAME)

test: build_test
	./build/$(TEST_EXEC_NAME)

debug: CFLAGS = -g -Og
debug: build/$(EXEC_NAME)

build/$(EXEC_NAME): $(OBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) $(OBJ) -o build/$(EXEC_NAME)

build_test: $(TEST_OBJ)
	@mkdir -p build/test
	$(CC) $(CFLAGS) $(TEST_OBJ) $(TEST_LDFLAGS) -o build/$(TEST_EXEC_NAME)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $^ -o $@

build/test/%.o: test/%.c
	@mkdir -p build/test
	$(CC) $(CFLAGS) -c $^ -o $@

bear: clean
	bear -- make all

clean:
	$(RM) -rf build
