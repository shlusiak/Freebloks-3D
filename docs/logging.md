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

## Events

`server_starting`, `listener_closed`, `listener_reopened`, `connection`,
`game_started`, `game_finished`, `player_result`, `game_terminating`, `chat`.
Lines without `event` are free-form messages (client joined/left, warnings
about rejected moves, errors).

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
