.PHONY: all test debug bear clean benchmark

CC:=gcc
CFLAGS:= -Werror -Wall -Wextra -Wpedantic -Wshadow -O3 -fopenmp -march=native -Isrc/external
TEST_FLAGS:= -Isrc/lib
MAIN_FLAGS:= -Isrc/lib
TEST_LDFLAGS:= -lcmocka

MAIN_SRC:=src/main.c
MAIN_OBJ:=build/main.o
LIB_SRC:=$(wildcard src/lib/*.c)
TEST_SRC:=$(wildcard test/*.c)
EXTERN_SRC:=$(wildcard src/external/*.c)
LIB_OBJ:=$(patsubst src/lib/%.c,build/lib/%.o,$(LIB_SRC))
TEST_OBJ:=$(patsubst test/%.c,build/test/%.o,$(TEST_SRC))
EXTERN_OBJ:=$(patsubst src/external/%.c,build/external/%.o,$(EXTERN_SRC))

EXEC_NAME:=conv
TEST_EXEC_NAME:=test_conv

all: build/$(EXEC_NAME)

test: build/$(TEST_EXEC_NAME)
	./build/$(TEST_EXEC_NAME)

debug: CFLAGS = -Isrc/external -g -Og
debug: build/$(EXEC_NAME)

benchmark: CFLAGS += -DBENCHMARK
benchmark: build/$(EXEC_NAME)

build/$(EXEC_NAME): $(MAIN_OBJ) $(LIB_OBJ) $(EXTERN_OBJ)
	@mkdir -p build
	@mkdir -p build/lib
	@mkdir -p build/external
	$(CC) $(CFLAGS) $(MAIN_FLAGS) $^ -o $@

build/$(TEST_EXEC_NAME): $(TEST_OBJ) $(LIB_OBJ) $(EXTERN_OBJ)
	@mkdir -p build
	@mkdir -p build/test
	$(CC) $(CFLAGS) $(TEST_FLAGS) $^ $(TEST_LDFLAGS) -o build/$(TEST_EXEC_NAME)

build/lib/%.o: src/lib/%.c
	@mkdir -p build
	@mkdir -p build/lib
	$(CC) $(CFLAGS) -c $^ -o $@

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(MAIN_FLAGS) -c $^ -o $@

build/test/%.o: test/%.c
	@mkdir -p build
	@mkdir -p build/test
	$(CC) $(CFLAGS) $(TEST_FLAGS) -c $^ -o $@

build/external/%.o: src/external/%.c
	@mkdir -p build	
	@mkdir -p build/external
	$(CC) $(CFLAGS) $(MAIN_FLAGS) -c $^ -o $@

bear: clean
	bear -- make all
	bear -- make test

clean:
	$(RM) -rf build
