# EventGamer

EventGamer is an adapter tool that connects telephones to a Snake implementation
in MadMapper, allowing players to control the game with their telephone keypad.

The tool is currently in early prototype stage.

```text
Telephones → Eventphone → SIP → EventGamer → OSC (UDP) → MadMapper / Snake
                                          ← game-over reply ←
```

Configure Eventphone to route calls to the SIP account used by EventGamer.
EventGamer answers incoming calls, queues callers, and translates the active
player's DTMF key presses into Snake controls: **2** = up, **4** = left,
**6** = right, **8** = down. Use RTP telephone events or SIP INFO for DTMF;
in-band tone detection is not supported.

Start with [EventGamer/example_configs/simple.toml](EventGamer/example_configs/simple.toml):

- `[sip]`: set `server` and `username` to your Eventphone SIP account details.
  Put the password on a single line in the file named by `password_file`, relative
  to the config file. Keep this password file out of version control.
- `[osc]`: set `remote_host` to the MadMapper machine and `remote_port` to its OSC
  input port. Set `bind_host` and `bind_port` for EventGamer's incoming OSC replies,
  and select `address_family` (`ipv4`, `ipv6`, or `auto`). The example uses ports
  9001 for MadMapper and 9000 for EventGamer.
- `[audio]`: set `in_queue`, `game_starts`, and `game_over` to the announcement
  WAV files (16-bit PCM mono). Relative paths resolve from the config directory.

Configure the MadMapper Snake implementation to accept `/snake/up`,
`/snake/left`, `/snake/right`, `/snake/down`, `/snake/restart`, and `/snake/stop`,
each carrying an OSC float `1.0`. Send `/snake_reply/game_over` back to
EventGamer's configured bind address and port when the game ends; use a reachable
host address rather than the wildcard `0.0.0.0`. EventGamer starts a game when a
player is attached, stops it when they detach, and plays the game-over announcement
before hanging up.

Build and run from the repository root:

```sh
./build_pjsua2.sh
cmake -S EventGamer -B EventGamer/build -G Ninja
cmake --build EventGamer/build
./EventGamer/build/EventGamer EventGamer/example_configs/simple.toml
```

See [EventGamer/README.md](EventGamer/README.md) for dependencies and build details,
and [MockMapper/README.md](MockMapper/README.md) for the OSC test receiver.

Copyright (C) 2026 Stefan Schuermans. Licensed under the GNU General Public
License, version 3 or later; see [LICENSE](LICENSE).
