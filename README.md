## Welcome to a chat-room made with C
- The guide to making the chat-room, both client and server side implementation
- First step establish a simple server, this will be written all in language
- All the clients at the moment will be implemented in python(cause its ease of use)

- generating new c styles that fit your needs:
  ```bash
  clang-format -style=llvm -dump-config > .clang-format
  # open up the .clang-format file and edit
  # the styles by looking for these options BreakBeforeBraces to either of this formatting:
  # Allman
  # GNU
  ```
- Simple implementation that will help you download all your dev dependencies(uv)l
  ```bash
  git archive --remote=git://github.com/libuv/libuv HEAD:deps/uv | tar -xv
  ```
