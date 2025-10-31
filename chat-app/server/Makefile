cc = clang
flags = -Wall -Werror -O2 -pedantic -std=c99 -g

poll = ./src/main.c ./src/core/poll_server.c
poll_obj = $(poll:.c=.o)

# compile epoll files only
epoll = ./src/epoll.c ./src/core/epoll_server.c ./src/utils/log.c
epoll_obj = $(epoll:.c=.o)

BIN_DIR = build
HEADER = $(wildcard ./src/includes/*.h)

# all: $(BIN_DIR)

poll:  $(poll_obj)
	$(CC) $^ -o $(BIN_DIR)/$@


epoll:  $(epoll_obj)
	$(CC) $^ -o $(BIN_DIR)/$@


clean:
	rm -rf ./build/build $(wildcard ./src/*.o) $(wildcard ./src/core/*.o) $(wildcard ./src/utils/*.o)

.PHONE: all clean
