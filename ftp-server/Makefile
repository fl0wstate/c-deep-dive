# Simple make file for compiling specific source files
# make a simple list in the make fi
cc = clang
flags = -Wall -Werror -pedantic -std=c99 -g
cli = ./cli_ftp_client.c ./log.c
cl = ./ftp-client.c ./log.c
ser = ./ftp-server.c ./log.c
obj1 = $(cli:.c=.o)
obj2 = $(cl:.c=.o)
obj3 = $(ser:.c=.o)
binary = bin

all: $(binary)

server: $(obj3) ftp.h
	$(cc) $(flags) $(obj3) -o $@


client: $(obj2) ftp.h
	$(cc) $(flags) $(obj2) -o $@


cli: $(obj1) ftp.h
	$(cc) $(flags) $(obj1) -o $@


$(binary): $(obj) ftp.h
	$(cc) $(flags) $(obj) -o $@


clean:
	rm -rf $(binary) server client cli $(wildcard ./*.o)

.PHONE: all clean server client cli

