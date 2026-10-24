# Logging

The server side (freebloks_dedicated and the shared game server code) writes
[logfmt](https://brandur.org/logfmt) lines: one event per line, `key=value`
pairs separated by single spaces. Output goes to stdout and, with
`--log <file>`, additionally to the file (appended). Each line is flushed
immediately and lines from concurrent games never interleave (process-wide
mutex in logger.cpp).

Client, GUI, benchmark, `--help` and argument errors are not part of this
format and still use plain console output.

## Fields

| Field | Presence | Meaning |
|---|---|---|
| ts | always | UTC, ISO 8601, millisecond precision, e.g. `2026-10-24T05:48:51.830Z` |
| level | always | `info`, `warn` or `error`. Informational only, there is no filtering. |
| game | in a game | Game number of the dedicated server (1, 2, ...). Absent for listener/startup lines. |
| client | per client | Client slot index inside the game |
| event | some lines | Stable machine-readable event name (see below) |
| addr | `connection` | Numeric peer address, no reverse DNS |
| player | `player_result` | Player (colour) index |
| msg | always | Human-readable text, last field |

Values containing anything other than `A-Za-z0-9 . _ - : / +` are double
quoted. Inside quotes `"` and `\` are backslash escaped, newline, CR and tab
become `\n`, `\r`, `\t`, other control characters `\xNN`. Bytes >= 0x80 are
passed through unchanged. `msg` is truncated to 1023 bytes.

## Message catalog

The following messages are emitted by the dedicated server and shared server
code. Text in square brackets is a runtime value; the actual `msg` text does
not include the brackets. `game` is present for game-scoped messages, while
`client` identifies a client slot where shown.

### Server and listener

| Level | Event / fields | Message | When it is emitted |
|---|---|---|---|
| info | `event=server_starting` | `Server starting` | Server startup, after optional file logging is configured. |
| info | — | `Logging to: [path]` | A log file was successfully opened. |
| info | `event=connection addr=[address]` | `Connection accepted` | A connection is accepted; address is numeric (no reverse DNS). |
| error | — | `Error calling listen: [reason]` | Initial listener setup or listener reopen fails. |
| error | — | `select failed: [reason]` | The listener wait fails for a persistent error. |
| error | — | `accept failed: [reason]` | Accepting a connection fails. |
| warn | — | `setsockopt IPV6_V6ONLY failed: [reason]` | Optional IPv6 socket configuration fails. |
| warn | — | `Too many open file descriptors, rejecting connection.` | An accepted descriptor cannot fit in the server's `select()` set. |
| warn | `event=listener_closed` | `Stopping accepting new connections due to too many running games` | The configured running-game limit is reached. |
| info | `event=listener_reopened` | `Resuming accepting new connections` | A game finishes and the listener resumes accepting clients. |
| error | — | `setregid failed: [reason]`; `setreuid failed: [reason]` | Dropping the configured group or user privileges fails; the server exits. |
| error | — | `pthread_create failed: [reason]` | Starting a game thread fails; that game is discarded. |
| error | — | `pthread_detach failed: [reason]` | Detaching a started game thread fails. |

### Game lifecycle and clients

| Level | Event / fields | Message | When it is emitted |
|---|---|---|---|
| info | `event=game_started` | `Game started with [clients] clients / [players] players. ([running] games running)` | A game thread begins running. |
| info | `event=game_finished` | `Game finished after [seconds] seconds` | The final player has finished; elapsed time is in seconds to two decimals. |
| info | `event=player_result player=[index]` | `Player [index] has [stones] stones left and [points] points` | Once per player at game completion. Player indices are colour slots. |
| info | `event=game_terminating` | `Game terminating. ([running] games running)` | The game thread is ending. |
| info | `client=[slot]` | `Client joined`; `Client left` | A client is added to or removed from the game. |
| error | `client=[slot]` | `close failed: [reason]` | Closing a client's socket fails. |
| error | `client=[slot]` | `Player already assigned (2 colors per player)` | A client attempts to claim a colour already assigned to another player; the client is disconnected. |
| error | — | `Game not in sync, local turn rejected. Exiting.` | The server's own turn fails validation; the game process exits with status 2. |

### Moves, chat, and protocol

| Level | Event / fields | Message | When it is emitted |
|---|---|---|---|
| warn | `client=[slot]` | `Ignoring move, no game running.` | A move arrives before there is a current player. |
| warn | `client=[slot]` | `Ignoring move from non-current player.` | The move is not from the current player. |
| warn | `client=[slot]` | `Client does not own player, ignoring move.` | The sending client does not own the selected player. |
| warn | `client=[slot]` | `Ignoring move with invalid stone or orientation.` | The stone or its orientation is invalid. |
| warn | `client=[slot]` | `Ignoring move with a stone that is not available.` | The player has already used that stone. |
| warn | `client=[slot]` | `Game seems to be out of sync, ignoring move.` | Applying a client move fails the server's consistency check. |
| info | `event=chat client=[slot]` | `[chat text]` | A chat message is received; the message body is the `msg` value. |
| error | `client=[slot]` | `Unhandled network message: #[type]` | The server receives an unrecognized network message type. |

Socket and thread errors append the platform's error text as `[reason]`.
Messages without an `event` are still structured logfmt records; their text is
intended for people, while the common fields and event names are suitable for
filtering and aggregation.

## Example

    ts=2026-10-24T05:48:51.830Z level=info event=connection addr=::1 msg="Connection accepted"
    ts=2026-10-24T05:48:51.830Z level=info client=0 msg="Client joined"
    ts=2026-10-24T05:48:51.831Z level=info game=1 event=game_started msg="Game started with 1 clients / 4 players. (1 games running)"
    ts=2026-10-24T05:48:53.574Z level=warn game=1 client=2 msg="Ignoring move from non-current player."

## Ingestion

- Loki / LogQL: `{job="freebloks"} | logfmt | level="error"`
- Datadog and Splunk parse `key=value` automatically; map `ts` as the timestamp attribute.
- OpenTelemetry Collector: `filelog` receiver with a `regex_parser`
  (`^ts=(?P<ts>\S+) level=(?P<level>\S+)`) plus `timestamp` and `severity` parsers. See the
  [filelog receiver docs](https://github.com/open-telemetry/opentelemetry-collector-contrib/tree/main/receiver/filelogreceiver).

## API (src/logger.h)

    logger->log(LOGLEVEL_WARN, "text %d", n);
    logger->logClient(LOGLEVEL_INFO, client, "text");
    logger->logFields(LOGLEVEL_INFO, "event=foo key=bar", "text");

`logFields` copies its `fields` argument verbatim: keys and values must be
trusted and contain no spaces, quotes or control characters. User-controlled
text goes into the `fmt` arguments only (`"%s"`), which are escaped.
