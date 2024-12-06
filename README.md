## Welcome to a chat-room made with C
```
chat-room/
├── src/
│   ├── main.c                # Entry point of the application
│   ├── tui.c                 # Handles the terminal UI (ncurses logic)
│   ├── tui.h                 # Header file for the TUI
│   ├── network.c             # Networking logic (socket connection, send/receive)
│   ├── network.h             # Header file for the networking functions
│   ├── message.c             # Handles message formatting and parsing
│   ├── message.h             # Header file for message-related utilities
│   ├── utils.c               # General utility functions
│   ├── utils.h               # Header file for utilities
│   └── client.c              # Client-side logic (combines TUI and networking)
├── server/
│   ├── server.c              # Server application (handles client connections)
│   ├── server_utils.c        # Utilities for the server (e.g., broadcast messages)
│   ├── server_utils.h        # Header file for server utilities
│   └── Makefile              # Makefile for compiling the server
├── include/
│   ├── common.h              # Shared constants (e.g., ports, buffer sizes)
│   ├── colors.h              # Defines color schemes for the TUI
│   └── config.h              # Configuration settings (e.g., server address)
├── build/                    # Compiled binaries (created after `make`)
│   ├── client
│   └── server
├── logs/                     # Optional: log files for debugging
├── Makefile                  # Top-level Makefile for building the entire project
└── README.md                 # Project overview and instructions
```
- The guide to making the chat-room, both client and server side implementation
- First step establish a simple server, integrate a client to it
- generating new c styles that fit your needs:
  ```bash
  clang-format -style=llvm -dump-config > .clang-format
  # open up the .clang-format file and edit the styles by looking for these options BreakBeforeBraces to either of this formatting:
  # Allman
  # GNU
  ```
- Simple implementation that will help you download all your dev dependencies(uv):
  ```bash
   # download all the uv dev dependencies
  git archive --remote=git://github.com/libuv/libuv HEAD:deps/uv | tar -xv
  ```
