# MockMapper

Portable C++17 OSC UDP mock, independently built beside EventGamer. It reuses
EventGamer's Asio transport, oscpack dependency setup, and termination handlers;
PJSUA2 and toml++ are not required.

```sh
cmake -S MockMapper -B MockMapper/build -G Ninja
cmake --build MockMapper/build
MockMapper/build/MockMapper ipv4 127.0.0.1 9001 127.0.0.1 9000
```

On Windows use `MockMapper/build/MockMapper.exe`. For IPv6, use e.g.
`ipv6 ::1 9001 ::1 9000`. Ports must be in 1–65535. Debian dependencies are
`build-essential cmake ninja-build libasio-dev liboscpack-dev`; CMake downloads
missing OSC libraries, using the same pinned revisions as EventGamer.

Received messages are dumped to stdout. Three identical consecutive OSC message
encodings (including address, argument types, and values) trigger a dumped outgoing
`/snake_reply/game_over` message containing one OSC float `1.0f`, sent to the
configured remote host/port. The counter resets after replying: six identical
messages produce two replies. A different message starts a new sequence.
Immediate bundles are unpacked in order, counting each message separately;
scheduled bundles are rejected. Malformed packets are logged without stopping
the server. Equality is byte-for-byte, so float NaNs and signed zero compare by
their transmitted representation. No game logic or message payload is modified.

Linux/macOS stop on SIGTERM, SIGINT, SIGPIPE, or SIGQUIT. Windows uses the same
console shutdown and C runtime signal handlers as EventGamer. Ctrl+C exits cleanly.

Run loopback tests with `ctest --test-dir MockMapper/build --output-on-failure`
(requires Python 3, with no extra Python packages).
