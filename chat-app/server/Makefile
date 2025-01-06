# Simple make file for compiling specific source files
# make a simple list in the make fi
cc = clang
flags = -Wall -Werror -pedantic -std=c99 -g
fl = ./src/full_server.c ./src/full_server_utils.c
obj = $(fl:.c=.o)
binary = bin

all: $(binary)

$(binary): $(obj) ./includes/server.h
	$(cc) $(flags) $^
clean:
	rm -rf $(binary) $(wildcard ./src/*.o)

.PHONE: all clean
