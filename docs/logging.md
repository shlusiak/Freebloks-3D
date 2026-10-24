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

Every line has `ts`, `level`, `event` and `msg`. Event names are a stable
contract: filter and alert on `event`, not on the text of `msg`.

| Field | Presence | Meaning |
|---|---|---|
| ts | always | UTC, ISO 8601, millisecond precision, e.g. `2026-10-24T05:48:51.830Z` |
| level | always | `info`, `warn` or `error`. Informational only, there is no filtering. |
| game | in a game | Game number of the dedicated server (1, 2, ...). Absent for listener/startup lines. |
| event | always | Stable snake_case event name (catalog below) |
| client | per client | Client slot index inside the game |
| reason | some events | Machine-readable cause token (see per-event values below) |
| errno | after a failed system call | Numeric `errno` value; the text is in `msg` |
| player | player events | Player (colour) index |
| msg | always | Human-readable text, last field |

Further fields are listed per event. Numeric fields are integers unless noted
(`duration_s` has two decimals). `mode` is the numeric `GAMEMODE` from
constants.h.

Values containing anything other than `A-Za-z0-9 . _ - : / +` are double
quoted. Inside quotes `"` and `\` are backslash escaped, newline, CR and tab
become `\n`, `\r`, `\t`, other control characters `\xNN`. Bytes >= 0x80 are
passed through unchanged. `msg` is truncated to 1023 bytes.

## Event catalog

`game` is present for game-scoped events. Fields in the third column are
in addition to the common ones.

### Server and listener

| Level | Event | Fields | When it is emitted |
|---|---|---|---|
| info | `server_starting` | `version port mode width height ai_strength ai_threads max_humans limit` | Server startup, after optional file logging is configured. |
| info | `log_file_opened` | | A log file was opened (path in `msg`). |
| info | `connection` | `addr family` (`ipv4`, `ipv6`, `other`) | A connection is accepted; address is numeric (no reverse DNS). |
| warn | `connection_rejected` | `reason=fd_limit` | An accepted descriptor does not fit in the server's `select()` set. |
| error | `listen_failed` | `errno` | Initial listener setup or listener reopen fails. |
| error | `select_failed` | `errno` | The listener wait fails for a persistent error. |
| error | `accept_failed` | `errno` | Accepting a connection fails. |
| error | `wait_failed` | `errno` | Waiting for players failed; the server exits. |
| warn | `socket_option_failed` | `option errno` | Optional IPv6 socket configuration fails. |
| warn | `listener_closed` | `games_running limit` | The configured running-game limit is reached. |
| info | `listener_reopened` | `games_running` | A game finishes and the listener resumes accepting clients. |
| error | `privilege_drop_failed` | `target` (`uid`, `gid`) `errno` | Dropping the configured user or group fails; the server exits. |
| error | `thread_failed` | `operation` (`create`, `detach`) `errno` | Starting or detaching a game thread fails. |
| error | `invalid_config` | `mode width height` | The game mode and field size are incompatible; the server exits. |
| error | `sigaction_failed` | `errno` | Installing the termination handlers fails; the server exits. |
| error | `stats_socket_failed` | `operation errno` | The stats socket cannot be created, bound, listened on or accepted from. |
| info | `shutdown_requested` | `signal` (number) | The main loop observed SIGTERM/SIGINT. |
| info | `shutdown_performed` | | The listener is closed and the process is about to exit. |

The signal handler itself writes `level=info event=signal_received signal=SIGTERM|SIGINT`
to stderr (without `ts`, as it must be async-signal-safe); the line is not
part of the stdout/file stream.

### Game lifecycle and clients

| Level | Event | Fields | When it is emitted |
|---|---|---|---|
| info | `game_started` | `clients players games_running mode width height ai_strength` | A game thread begins running. |
| info | `game_finished` | `duration_s turns` | The final player has finished. |
| info | `player_result` | `player stones_left points computer` (0/1) | Once per player at game completion. Player indices are colour slots. |
| info | `game_terminating` | `reason=all_clients_left games_running` | The game thread is ending. |
| info | `client_joined` | `client clients` | A client is added to the game; `clients` is the count including it. |
| info | `client_left` | `client reason clients` | A client is removed. `reason`: `peer_closed`, `invalid_package`, `recv_failed`, `send_failed`, `protocol_violation`. |
| error | `client_close_failed` | `client errno` | Closing a client's socket fails. |
| error | `player_claim_rejected` | `client player` | A client claims a colour already assigned to another player; the client is disconnected. |
| error | `out_of_sync` | `source=local_turn player` | The server's own turn fails validation; the process exits with status 2. |

### Moves, chat, and protocol

| Level | Event | Fields | When it is emitted |
|---|---|---|---|
| warn | `move_rejected` | `client reason player stone` | A client move is ignored. `reason`: `no_game`, `not_current`, `not_owner`, `invalid_stone`, `stone_used`, `out_of_sync`. `player`/`stone` are present when known. |
| info | `chat` | `client` | A chat message is received; the message body is the `msg` value. |
| warn | `invalid_package` | `client` | A client sent a malformed, oversized or mismatched message; it is disconnected. |
| warn | `recv_failed` | `client errno` | Reading from a client failed for another reason; it is disconnected. |
| warn | `send_failed` | `client msg_type errno` | Sending to a client failed (`errno` 0 means the peer had closed); it is disconnected. |
| error | `unhandled_message` | `client msg_type` | The server receives an unrecognized network message type. |

Socket and thread errors append the platform's error text to `msg`.

## Example

    ts=2026-10-24T05:48:51.830Z level=info event=connection addr=::1 family=ipv6 msg="Connection accepted"
    ts=2026-10-24T05:48:51.830Z level=info event=client_joined client=0 clients=1 msg="Client joined"
    ts=2026-10-24T05:48:51.831Z level=info game=1 event=game_started clients=1 players=4 games_running=1 mode=2 width=20 height=20 ai_strength=5 msg="Game started with 1 clients / 4 players. (1 games running)"
    ts=2026-10-24T05:48:53.574Z level=warn game=1 event=move_rejected client=2 reason=not_current player=1 msg="Ignoring move from non-current player."

## Ingestion

- Loki / LogQL: `{job="freebloks"} | logfmt | level="error"`
- Datadog and Splunk parse `key=value` automatically; map `ts` as the timestamp attribute.
- OpenTelemetry Collector: `filelog` receiver with a `regex_parser`
  (`^ts=(?P<ts>\S+) level=(?P<level>\S+)`) plus `timestamp` and `severity` parsers. See the
  [filelog receiver docs](https://github.com/open-telemetry/opentelemetry-collector-contrib/tree/main/receiver/filelogreceiver).

## Containers

The Docker image logs to stdout only; no log file or volume is configured.
Collect the stream with the Docker log driver and bound its size, as in
docker-compose.yml (`json-file` with `max-size`/`max-file`), or use a driver
such as `journald` or `loki`. Set the `LOGFILE` environment variable only if a
file is wanted in addition. The server never reopens its log file, so external
rotation requires `copytruncate`.

## API (src/logger.h)

    logger->logEvent(LOGLEVEL_WARN,
        LogFields("move_rejected").client(c).tok("reason", "not_current").num("player", p),
        "text %d", n);

`LogFields` builds `event=... key=value ...`. `tok()` values and all keys are
copied verbatim and must be trusted constants without spaces, quotes or control
characters. User-controlled text goes into the `fmt` arguments only (`"%s"`),
which are escaped. Every call must have an event name.
