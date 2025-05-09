# Simple make file for compiling specific source files
# make a simple list in the make fi
cc = gcc
flags = -Wall -Werror -Wno-overlength-strings -pedantic -std=c89 -g
cli = ./cli_ftp_client.c ./log.c ./utils.c
cl = ./ftp-client.c ./log.c # don't forget to add the utils.c
ser = ./ftp-server.c ./log.c ./utils.c
obj1 = $(cli:.c=.o)
obj2 = $(cl:.c=.o)
obj3 = $(ser:.c=.o)
binary = server cli

all: $(binary)

server: $(obj3) ftp.h
	$(cc) $(flags) $(obj3) -lpthread -o $@


client: $(obj2) ftp.h
	$(cc) $(flags) $(obj2) -lpthread -o $@


cli: $(obj1) ftp.h
	$(cc) $(flags) $(obj1) -lpthread -o $@


%.o: *%.c ftp.h
	$(cc) $(flags) -lpthread -c $< -o $@

#$(binary): $(obj) ftp.h
	# $(cc) $(flags) $(obj) -o $@


clean:
	rm -rf $(binary) server client cli $(wildcard ./*.o) $(wildcard ./*.out)

.PHONY: all clean server client cli

