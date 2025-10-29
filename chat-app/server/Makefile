cc = clang
flags = -Wall -Werror -pedantic -std=c99 -g
fl = ./src/main.c ./src/core/full_server_utils.c
obj = $(fl:.c=.o)
BIN_DIR = build
HEADER = ./includes/server.h

all: $(BIN_DIR)

$(BIN_DIR):  $(obj)
	$(CC) $^ -o $(BIN_DIR)/$@


clean:
	rm -rf ./build/build $(wildcard ./src/*.o) $(wildcard ./src/core/*.o)

.PHONE: all clean
