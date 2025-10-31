## Learning roadmap (ordered, small steps with success criteria)

## Phase 0 — Environment & basics

#### Task 0.1: Create minimal build/run environment

- [x] Create a small Makefile or compile command (`gcc -Wall -Wextra -O2`).
- [x] Success: You can compile and run a program that prints “hello”.

#### Task 0.2: Get comfortable with tools

- [x] Install `websocat`, `tcpdump/wireshark`, `strace`, and set `ulimit` -n to a high number.
- [x] Success: You can connect with `wscat` to a real server (e.g., `echo.websocket.org` or a local test server).

## Phase 1 — Epoll fundamentals (hands-on)

- [ ] Task 1.1: Non-blocking accept loop (no `WebSocket`)
  - Implement: create listening socket (`SOCK_NONBLOCK`), bind, listen, and an `epoll` loop that accepts connections and echoes bytes back.
  - Keep it tiny — only echo small payloads.
  - Success criteria:
    - Multiple concurrent clients can connect (using `telnet/wscat` raw TCP) and send lines; server echoes them.
    - Use `strace` or logs to verify minimal `syscalls`:
      `epoll_wait`, `accept`, `read`, `write`.
  - Hints:
    - Use `accept4(..., SOCK_NONBLOCK)`.
    - After accept, set `TCP_NODELAY`.
    - Use `epoll` in level-triggered mode first.

- [ ] Task 1.2: Add graceful signal handling
  - Use `signalfd` in the loop to stop the server gracefully.
  - Success: `SIGINT` causes server to close listeners and exit cleanly.

### Phase 2 — HTTP Upgrade handshake (learn RFC pieces)

---

- [ ] Task 2.1: Handshake parser
  - Implement small parser to accept an HTTP Upgrade request and compute `Sec-WebSocket-Accept`.
  - Success:
    - Use curl to send a handshake; server replies with 101 Switching Protocols and correct `Sec-WebSocket-Accept`.
  - Hints:
    - `Sec-WebSocket-Accept = base64( SHA1( client_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" ) )`.
    - Only accept GET, check headers: Upgrade: `websocket`, Connection: Upgrade, `Sec-WebSocket-Key present`.
- [ ] Test approach:
  - Use curl to send a crafted HTTP Upgrade and check reply.

### Phase 3 — Basic WebSocket frames (no fragmentation)

---

- [ ] Task 3.1: Frame parser for small client frames
  - Implement parsing for frames where payload_len < 126 and masked.
  - Beware: client frames are masked — unmask before use.
  - Success:
    - wscat can send small text messages and server echoes them back (server constructs unmasked server frames).
  - Hints:
    - Read 2-byte header: FIN/opcode + mask/payload_len.
    - If MASK = 1, read 4-byte mask then payload and unmask with mask[i % 4].
- Task 3.2: Write buffering and backpressure
  - Implement a per-connection write buffer (simple ring or vector of slices).
  - When send returns EAGAIN or partial, store remainder and register EPOLLOUT.
  - Success:
    - Slow client causes writes to queue up; server doesn’t crash and continues to serve others.
  - Hints:
    - Limit per-connection buffered bytes (e.g., 1–4 MB). If exceeded, disconnect or shed load.

### Phase 4 — Control frames & fragmentation

---

- Task 4.1: PING/PONG and CLOSE handling
  - Implement immediate processing for control frames: opcode 0x8/0x9/0xA.
  - Success:
    - Server replies to PING with PONG; CLOSE triggers close handshake.
- Task 4.2: Fragmentation/continuation frames
  - Implement incremental reassembly: track message buffer across frames, respecting FIN/continuation opcodes.
  - Success:
    - Clients that send fragmented messages are handled correctly.

### Phase 5 — Robustness, timeouts, TLS

---

- Task 5.1: Idle timeouts & PING-based keepalive
  - Use timerfd to periodically check last_activity and send PINGs or close timed-out connections.
- Task 5.2: Non-blocking TLS (optional, harder)
  - Integrate OpenSSL in non-blocking mode. Handle SSL_ERROR_WANT_READ/WRITE by mapping to EPOLLIN/EPOLLOUT.
  - Success:
    - Browser connects with wss:// and protocol behaves.

Testing & validation approach (TDD-like)

- I’ll give you small, runnable test harnesses you can use locally for these focused tasks (for example, a handshake validator that takes an HTTP request and returns the Accept string). If you want a test harness for a specific task, say which task and I’ll provide the test file + minimal harness (not the full server).
- Use wscat and websocat for live testing. For automated checks, write small unit tests around parsing/unmasking functions.

Debugging checklist (useful when things go wrong)

- Is socket non-blocking? (O_NONBLOCK or SOCK_NONBLOCK)
- Are you checking EAGAIN/EWOULDBLOCK for recv/send?
- Are you registering the correct events (EPOLLIN vs EPOLLOUT)? Avoid re-adding too often.
- Are you unmasking client frames?
- Are you handling partial reads/writes? Track offsets.
- For TLS: are you respecting SSL_ERROR_WANT_READ/WRITE?
- For fragmentation: are you concatenating payloads in correct order and resetting state after complete message?
- Use strace -f -e trace=network -s 200 for syscall tracing and tcpdump/wireshark for packet-level.

Concrete ways I can help next (pick one)

- Option A — I give you a set of unit tests for the WebSocket handshake and a tiny test harness (not the full server) so you can implement the handshake and run tests locally. I will provide tests only, not implementation.
- Option B — I guide you step-by-step while you implement Task 1.1 (non-blocking accept + epoll echo). You implement, paste code here, I’ll review and explain fixes.
- Option C — I provide a “fill-in-the-blanks” header file for frame parsing with TODOs (no implementation), plus test vectors (example raw frames) that you must decode.
- Option D — I just give you deeper hints and pseudocode for any single task you pick (e.g., write buffering logic, fragmentation state machine, or non-blocking OpenSSL pattern).

What I’ll do next when you pick an option

- If you pick A/C: I’ll immediately produce the test files or TODO file (as small files) so you can run them locally.
- If you pick B: I’ll wait for you to paste your code or ask for the first hint; then I’ll review and iterate.
- If you pick D: I’ll post focused pseudocode and a short checklist for that task.

Which option do you want me to do now? (A, B, C, or D and which task)
