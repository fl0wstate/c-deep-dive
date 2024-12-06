# Simple make file for compiling specific source files
# make a simple list in the make fi
cc = clang
flags = -Wall -Werror -pedantic -std=c99 -g
obj = $(fl:.c=.o)
binary = bin

all: $(binary)

$(binary): $(obj)
	$(cc) $(flags) -o $@ $^

clean:
	rm -rf $(binary) $(wildcard *.o)

.PHONE: all clean
