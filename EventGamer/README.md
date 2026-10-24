# EventGamer

C++17 application with TOML configuration parsing and PJSUA2 SIP registration.
Incoming calls are answered automatically. Audio plays only when explicitly requested.
OSC UDP transport and typed message dispatch are implemented; game command addresses
and MadMapper-specific behavior remain to be defined.

Build from the repository root with CMake 3.20+, Ninja, Git, and a C++17 compiler:

```sh
./build_pjsua2.sh
cmake -S EventGamer -B EventGamer/build -G Ninja
cmake --build EventGamer/build
cp EventGamer/example_configs/sip-password.txt.example EventGamer/example_configs/sip-password.txt
# Edit sip-password.txt to contain your SIP password before running.
./EventGamer/build/EventGamer EventGamer/example_configs/simple.toml
```

On Windows, the executable is `EventGamer/build/EventGamer.exe`.
On Linux/macOS, the top-level `build_pjsua2.sh` downloads PJPROJECT 2.17, builds
PJSUA2 and its bundled dependencies, and installs into `.deps/pjproject/install`.
Sources and build products also live under the gitignored `.deps/` directory.
It requires Git, GNU Make, pkg-config, and C/C++ compilers; on Debian install
these with `sudo apt install build-essential git pkg-config`. No sudo is used
by the script. Set `JOBS=4` to limit build parallelism. Rerunning the script
reuses the existing checkout and rebuilds/installs it. Video and physical sound
device support are disabled; audio files can still be played into calls.

CMake automatically prefers this local installation and uses its pkg-config
metadata, including static link dependencies. Override the prefix with
`-DPJPROJECT_ROOT=/another/install/prefix`. Without a local installation it
searches for a system PJPROJECT installation. For other non-system prefixes,
you can also set `PKG_CONFIG_PATH` to the appropriate `lib/pkgconfig` directory.

The download/build script targets Linux/macOS. On Windows, either use a pkg-config installation
or pass `-DPJPROJECT_INCLUDE_DIR=...` and
`-DPJPROJECT_LIBRARIES="...;..."` with the SDK's complete library list (including
system and third-party dependencies), built for the same compiler and architecture.
CMake first looks for an installed toml++ package (version 3.4 or newer).
On Debian, install it with `sudo apt install libtomlplusplus-dev`.
If none is found, configuration downloads toml++ v3.4.0 from GitHub. For an offline build,
provide a local checkout using CMake's
`-DFETCHCONTENT_SOURCE_DIR_TOMLPLUSPLUS=/path/to/tomlplusplus` option.

All settings shown in `example_configs/simple.toml` are required. Unknown keys
are rejected. `osc.address_family` accepts exactly `ipv4`, `ipv6`, or `auto`;
both ports must be integers from 1 to 65535. Audio paths are resolved relative
to the configuration file. Audio file existence and WAV contents are not
validated yet. Keep actual passwords out of version control.

`sip.password_file` is required and must be a relative path, resolved against
the TOML file's directory. Its text file contains one nonempty password line.
An optional final LF or CRLF is removed; spaces are preserved. Empty, unreadable,
or multiline files are rejected without including their contents in errors.
The example's `sip-password.txt` is gitignored; its `.example` template contains
only a placeholder. The old inline `sip.password` key is no longer accepted.

The executable validates configuration, starts UDP SIP on an OS-selected local
port, and registers with the configured server using digest authentication.
`sip.server` is a hostname or IP address with an optional port, without a `sip:`
prefix. The same server is used as the account domain. This initial version
creates an IPv4 UDP transport; use an IPv4-reachable server. The OSC address
family setting does not control SIP.

PJSUA2 refreshes registration automatically and retries eligible transient failures
after 30 seconds (initial transport-failure retry after 5 seconds). Registration
status is printed; rejected credentials must be fixed in the config and the app
restarted. The main thread polls SIP events at intervals up to 10 ms.
No physical sound device is required.

For SIP diagnostics, run `EventGamer <config.toml> --sip-trace`. This prints
incoming/outgoing SIP messages, including addressing and authentication headers;
redact those headers before sharing a trace. Normal logging reports the bound
and advertised SIP address, incoming calls, rejected calls, and queue playback.

Each incoming call is represented by an `ActiveCall` with its own WAV player. All sounds use non-looping playback.
Calls are kept in a vector in acceptance order, oldest first. No player is created on acceptance. `Client::playAudio()` selects a configured
sound and starts it when audio media is active.
Disconnected calls are removed by the main loop without changing the order of
remaining calls. Queue WAV files must be supported by PJSUA2 (16-bit PCM mono).
Concurrent call capacity uses PJSUA2's default limit (normally four calls);
increasing it will require endpoint configuration and possibly rebuilding PJPROJECT.

DTMF events print `SIP call <id>: key 9 pressed`. RTP telephone events
(RFC 2833/4733) log the start of each digit; intermediate duration updates and
release notifications are ignored. SIP INFO logs a press when its completed digit
message arrives. In-band audio tone detection is not implemented. These events describe received DTMF;
phones or PBXs may generate fixed-duration tones instead of tracking a held key.

On Linux and macOS, SIGTERM, SIGINT, SIGPIPE, or SIGQUIT requests shutdown.
On Windows, Ctrl+C, Ctrl+Break, console close/logoff/shutdown notifications, and
C runtime SIGINT/SIGTERM request shutdown. Windows can impose a deadline or omit
some console notifications; forced process termination cannot run cleanup.
Normal shutdown destroys the SIP account and endpoint, allowing unregistration,
and exits with status 0. Startup/runtime errors exit with status 1.
Configuration values and credentials are not printed on successful config loading.
TOML syntax errors include parser diagnostics and can contain source excerpts.

Open the repository root in VS Code and install the recommended clangd and
CMake Tools extensions. The workspace enables C++ format-on-save using clangd's
clang-format engine and the root `.clang-format` style. Install `clangd` on your
PATH (on Debian: `sudo apt install clangd`). Configure CMake once to generate
`EventGamer/build/compile_commands.json`, which clangd discovers automatically.

When a sound finishes, `Logic::audioFinished(AudioKey)` is called once
on the main thread. The media-thread EOF callback only sets an atomic flag, so
logic can safely start a new sound or hang up. Replacing a sound interrupts it
without a completion notification. QueueLogic and GameLogic currently leave
this callback empty.

OSC uses standalone Asio for UDP and oscpack for message encoding/decoding.
CMake prefers installed copies; on Debian use
`sudo apt install libasio-dev liboscpack-dev`. Otherwise CMake downloads Asio
1.30.2 and a pinned oscpack revision under the ignored build directory.
Offline source overrides are `FETCHCONTENT_SOURCE_DIR_ASIO` and
`FETCHCONTENT_SOURCE_DIR_OSCPACK`.

The main loop owns `OSCTransport`, then `GameProtocol`, then the shared
`GameLogic`. Each dependent object stores a reference to its dependency, which
outlives it. OSC is polled on the main thread, processing up to 64 datagrams per
iteration. Hostnames are resolved at startup; `auto` selects one family common
to the local and remote addresses, rather than opening both IPv4 and IPv6 sockets.
The bound socket is also used for outgoing messages.

`GameProtocol::sendMessage()` accepts an OSC address and typed arguments
(int32, float, string, bool, int64, double). Incoming messages are decoded into
`OSCMessage` and dispatched to an exact address handler registered with
`setHandler()`. An empty handler removes a registration. Unhandled addresses are
logged. For example (illustrative addresses, not a defined game protocol):

```cpp
protocol.setHandler("/game/status", [](const OSCMessage& message) {
    // Validate argument types and interpret the status here.
});
protocol.sendMessage({"/game/key", {std::string{"9"}}});
```

Immediate bundles are supported, including nesting up to 16 levels. Scheduled
bundles and unsupported argument types are rejected. Malformed packets are
logged and do not stop subsequent reception. UDP send errors propagate to the
caller; delivery, acknowledgment, and retransmission are not provided.
GameLogic holds the GameProtocol reference, ready for command/status behavior.

Run the OSC loopback tests with:

```sh
ctest --test-dir EventGamer/build --output-on-failure
```
