# Freebloks network protocol

Wire format of every message exchanged between a Freebloks client and server.

Source of truth: [src/network.h](../src/network.h), [src/network.cpp](../src/network.cpp), message handling in [src/spielserver.cpp](../src/spielserver.cpp) (`CSpielServer::process_message`) and [src/spielclient.cpp](../src/spielclient.cpp) (`CSpielClient::process_message`). Sizes below were verified by compiling `network.h` and printing `sizeof` / `offsetof`.

## Transport

- TCP, default port `59995` (`TCP_PORT`).
- No handshake, no authentication, no encryption. The server sends a `MSG_CHAT` ("Client N joined") and a `MSG_SERVER_STATUS` immediately after `accept()`; the client may send at any time.
- Messages are sent back-to-back on the byte stream. Each message is self-delimiting through the header's `data_length`.
- Maximum accepted message size: 512 bytes on both server and client (receive buffer size). Larger messages are treated as invalid and the connection is dropped.

## Conventions

- All structs are `#pragma pack(1)`: no padding, fields are packed in declaration order.
- `data_length` is big-endian (network byte order, `htons`/`ntohs`). All other fields are single bytes.
- `int8` is `signed char`, `uint8` is `unsigned char`.
- Strings are fixed-size byte arrays, NUL-terminated by convention, no defined character set (treat as raw bytes, typically ASCII/Latin-1).
- Player numbers: `0..3` (`PLAYER_MAX = 4`). Client indices: `0..7` (`CLIENTS_MAX = 8`).
- Stone numbers: `0..20` (`STONE_COUNT_ALL_SHAPES = 21`).

### Header (`NET_HEADER`, 5 bytes)

Every message starts with this header.

| Offset | Size | Field         | Description |
|-------:|-----:|---------------|-------------|
| 0      | 1    | `check1`      | Checksum 1 |
| 1      | 2    | `data_length` | **Total** message length in bytes, header included. Big-endian. |
| 3      | 1    | `msg_type`    | Message type, see table below |
| 4      | 1    | `check2`      | Checksum 2 |

Checksums (`network_send` / `network_check_message`), all arithmetic truncated to 8 bits:

```
check1 = (uint8)(data_length & 0x55) ^ msg_type
check2 = (uint8)((uint8)(check1 ^ 0xD6) + msg_type)
```

`data_length` here is the host-order value. Only the low byte of `data_length` influences `check1` (mask `0x55`). The checksums detect framing garbage; they are not integrity protection.

Receive procedure (`read_network_message`):

1. Read exactly 5 bytes. Fewer: invalid, connection dropped. Zero bytes: peer closed.
2. Reject if `data_length < 5` or either checksum mismatches.
3. If `data_length == 5`, the message is complete.
4. Reject if `data_length > 512`.
5. Read exactly `data_length - 5` more bytes in a single `recv()`. A short read is treated as invalid and drops the connection (no reassembly loop).

Any receive error closes the connection (server: `delete_client`, client: `Disconnect`).

### Message types

| Value | Name                       | Struct                   | Direction          | Total size (bytes) |
|------:|----------------------------|--------------------------|--------------------|-------------------:|
| 1     | `MSG_REQUEST_PLAYER`       | `NET_REQUEST_PLAYER`     | client -> server   | 22 (5 for legacy)  |
| 2     | `MSG_GRANT_PLAYER`         | `NET_GRANT_PLAYER`       | server -> client   | 6                  |
| 3     | `MSG_CURRENT_PLAYER`       | `NET_CURRENT_PLAYER`     | server -> client   | 6                  |
| 4     | `MSG_SET_STONE`            | `NET_SET_STONE`          | both               | 11                 |
| 5     | `MSG_START_GAME`           | `NET_START_GAME`         | both               | 5                  |
| 6     | `MSG_GAME_FINISH`          | `NET_GAME_FINISH`        | server -> client   | 5                  |
| 7     | `MSG_SERVER_STATUS`        | `NET_SERVER_STATUS`      | server -> client   | 171 (version 3)    |
| 8     | `MSG_CHAT`                 | `NET_CHAT`               | both               | 9 + text length    |
| 9     | `MSG_REQUEST_UNDO`         | `NET_REQUEST_UNDO`       | client -> server   | 5                  |
| 10    | `MSG_UNDO_STONE`           | `NET_UNDO_STONE`         | server -> client   | 11                 |
| 11    | `MSG_REQUEST_HINT`         | `NET_REQUEST_HINT`       | client -> server   | 6                  |
| 12    | `MSG_STONE_HINT`           | `NET_SET_STONE`          | server -> client   | 11                 |
| 13    | `MSG_REVOKE_PLAYER`        | `NET_REVOKE_PLAYER`      | both               | 6                  |
| 14    | `MSG_REQUEST_GAME_MODE`    | `NET_REQUEST_GAME_MODE`  | client -> server   | 30                 |

Unknown types are logged and ignored by both sides (`default:` branch); the connection stays open.

## Messages

### 1 `MSG_REQUEST_PLAYER` (client -> server), 22 bytes

Ask the server for a player (colour slot) in the lobby.

| Offset | Size | Field    | Description |
|-------:|-----:|----------|-------------|
| 0      | 5    | `header` | |
| 5      | 1    | `player` | `int8`. Preferred player, or `-1` for any. Added in 1.5. |
| 6      | 16   | `name`   | Client name, NUL-terminated (the sender forces `name[15] = 0`). Empty string: name unchanged. |

Server behaviour:

- Messages shorter than 22 bytes are dropped silently. A header-only (5 byte) request is therefore ignored by the current server, despite the `data_length > sizeof(NET_HEADER)` checks that anticipate it.
- Dropped silently if the game is running (`current_player != -1`), if the number of humans is already `max_humans`, or if all slots are taken.
- Modes `2_COLORS_2_PLAYERS`, `DUO`, `JUNIOR`: only players `0` and `2` are assignable. A preferred player other than `0`/`2` is ignored (random pick); a taken preferred slot falls over to the other one.
- Other modes: a preferred player in `0..4` is honoured (note: `4` passes the range check and indexes `player[4]`, out of range; senders must use `0..3` or `-1`); otherwise random. Taken slot falls over to the next free one.
- Mode `4_COLORS_2_PLAYERS`: the client is granted a second player `(n + 2) % 4` as well (two `MSG_GRANT_PLAYER` messages).
- On success: `MSG_GRANT_PLAYER` to the requester (one per granted slot), then `MSG_SERVER_STATUS` to everybody.

### 2 `MSG_GRANT_PLAYER` (server -> client), 6 bytes

Server assigns a player to the receiving client.

| Offset | Size | Field    | Description |
|-------:|-----:|----------|-------------|
| 0      | 5    | `header` | |
| 5      | 1    | `player` | `int8`, `0..3` |

Also sent unsolicited when a game-mode change (`MSG_REQUEST_GAME_MODE`) moves a client to a different slot.

### 3 `MSG_CURRENT_PLAYER` (server -> client), 6 bytes

Whose turn it is. Broadcast after game start, after every accepted move, after undo, and after a rejected move (resynchronisation).

| Offset | Size | Field    | Description |
|-------:|-----:|----------|-------------|
| 0      | 5    | `header` | |
| 5      | 1    | `player` | `int8`. `0..3`, or `-1` when no game is running / game over. |

### 4 `MSG_SET_STONE` (both), 11 bytes

Client -> server: request to place a stone. Server -> client: authoritative broadcast of an accepted move (including the originating client; computer moves use the same message). The server re-broadcasts the received packet unchanged after validation.

| Offset | Size | Field          | Description |
|-------:|-----:|----------------|-------------|
| 0      | 5    | `header`       | |
| 5      | 1    | `player`       | `int8`. Player placing the stone. |
| 6      | 1    | `stone`        | `uint8`. Stone shape number, `0..20`. |
| 7      | 1    | `mirror_count` | `uint8`. Mirror state applied to the stone (`CStone::mirror_rotate_to`). |
| 8      | 1    | `rotate_count` | `uint8`. Rotation state applied to the stone. |
| 9      | 1    | `x`            | `int8`. Board column. |
| 10     | 1    | `y`            | `int8`. Board row. |

Server validation of a client message:

- Length `>= 11`, else dropped.
- `player` must equal the current player, else the move is ignored and `MSG_CURRENT_PLAYER` is re-sent.
- The sending client must own that player (`clients[client] == player[player]`), else ignored and `MSG_CURRENT_PLAYER` re-sent.
- The move must be legal (`is_valid_turn` + `set_stone`). Illegal: ignored (logged as out of sync), `MSG_CURRENT_PLAYER` re-sent.
- Legal: broadcast `MSG_SET_STONE`, advance to the next player that has a legal move, broadcast `MSG_CURRENT_PLAYER`. If no player has a legal move, `MSG_GAME_FINISH` is broadcast and `current_player` becomes `-1`.

A client that receives an illegal `MSG_SET_STONE` from the server exits (`Game not in sync!`).

### 5 `MSG_START_GAME` (both), 5 bytes (header only)

- Client -> server: request to start the game. Ignored if a game is already running.
- Server -> client: a new round starts. The client resets its board using the last received `MSG_SERVER_STATUS` (game mode, stone counts) and clears its move history.

Server start sequence: `MSG_SERVER_STATUS`, `MSG_START_GAME`, `MSG_CURRENT_PLAYER` (player `0` always starts).

### 6 `MSG_GAME_FINISH` (server -> client), 5 bytes (header only)

No player has a legal move left. Always followed by `MSG_CURRENT_PLAYER` with `player = -1`.

### 7 `MSG_SERVER_STATUS` (server -> client), 171 bytes

Lobby/game state snapshot. Broadcast on client join, on client leave, after `MSG_REQUEST_PLAYER`, `MSG_REVOKE_PLAYER`, `MSG_REQUEST_GAME_MODE`, at game start, and as a heartbeat roughly every 30 seconds while idle (the heartbeat exists to provoke send errors on dead connections).

| Offset | Size | Field                    | Description |
|-------:|-----:|--------------------------|-------------|
| 0      | 5    | `header`                 | |
| 5      | 1    | `player`                 | `int8`. Number of human players. |
| 6      | 1    | `computer`               | `int8`. Number of computer players (`max - humans`, where `max` is 2 for 2-player modes, else 4). |
| 7      | 1    | `clients`                | `int8`. Number of connected clients. |
| 8      | 1    | `width`                  | `int8`. Board width. |
| 9      | 1    | `height`                 | `int8`. Board height. |
| 10     | 5    | `stone_numbers_obsolete` | `int8[5]`. Obsolete. The server always sends `1` in each. Only used by version <= 2 logic on the client. |
| 15     | 1    | `gamemode`               | `int8`. See [game modes](#game-modes). |
| 16     | 4    | `spieler`                | `int8[4]`. Owner of each player slot: `-2` computer, otherwise the index (`0..7`) of the owning client. (`-1` = `PLAYER_LOCAL` is a client-internal value.) Added in 1.5 (version 2). |
| 20     | 128  | `client_names`           | `uint8[8][16]`. Name of each client, NUL-terminated, empty string for unnamed/unused slots. Added in 1.5 (version 2). |
| 148    | 1    | `version`                | `int8`. Currently `3`. Added in 1.6 (version 3). |
| 149    | 1    | `version_min`            | `int8`. Minimum message version the server supports. Currently `1`. |
| 150    | 21   | `stone_numbers`          | `int8[21]`. How many copies of each stone shape each player has. Added in 1.6 (version 3). |

Client-side version detection (`CSpielClient::process_message`) is length based:

| `data_length` | Interpreted version |
|---------------|---------------------|
| `< 148`       | 1 (no `spieler`, no names) |
| `>= 148`      | 2 |
| `>= 150`      | the `version` byte of the message (3 today) |

The constants in the client are `143 + 5` and `143 + 7`; `143 + 5 = 148` equals the offset of `version` in the current layout. The `data_length < 148` branch describes the legacy version 1 layout, which does not match the current struct.

If `width` / `height` differ from the client's board, the client resizes and restarts its local board.

### 8 `MSG_CHAT` (both), 9 + n bytes

| Offset | Size | Field    | Description |
|-------:|-----:|----------|-------------|
| 0      | 5    | `header` | |
| 5      | 1    | `client` | `int8`. Sender. Ignored on client -> server messages; the server overwrites it with the real client index. `-1` = server message. |
| 6      | 1    | `length` | `uint8`. Length of the text in bytes, excluding NUL. |
| 7      | n+2  | `text`   | Text bytes. |

Length details (from `CSpielClient::chat` and `CSpielServer::send_server_msg`): senders allocate `sizeof(NET_CHAT) + strlen(text) + 1 = 9 + n` bytes, with `sizeof(NET_CHAT) = 8` already containing one byte of `text[1]`. So `data_length = 9 + n`, and the text area holds the `n` characters, a NUL, and one extra trailing byte. The server forces the last byte (`text[data_length - 8]`) to `0` before broadcasting. Receivers must use `data_length` or NUL termination, not trust `length` alone (`length` is a `uint8`; effective maximum `n` is bounded by the 512-byte message limit, about 503).

Server behaviour: messages shorter than 8 bytes are dropped; otherwise `client` is set, the text is terminated, and the packet is broadcast to **all** clients including the sender. The server also emits `MSG_CHAT` with `client = -1` for "Client N joined" and similar notices.

### 9 `MSG_REQUEST_UNDO` (client -> server), 5 bytes (header only)

Request to take back moves. Ignored when more than one client **and** more than one human player are present. Otherwise the server repeatedly sends `MSG_UNDO_STONE` and pops moves until a human player is on turn again (or history is empty), then sends `MSG_CURRENT_PLAYER`.

### 10 `MSG_UNDO_STONE` (server -> client), 11 bytes

Same struct as `NET_SET_STONE`, but its payload fields are **not initialised** by the server (stack garbage). Receivers must ignore the content and undo the most recent move from their own history (`CSpielClient` does exactly this).

### 11 `MSG_REQUEST_HINT` (client -> server), 6 bytes

| Offset | Size | Field    | Description |
|-------:|-----:|----------|-------------|
| 0      | 5    | `header` | |
| 5      | 1    | `player` | `int8`. Player to compute a hint for. |

The server runs the AI at `KI_HARD` and answers **only the requester** with `MSG_STONE_HINT`. No answer if the AI finds no turn. The server does not check that `player` belongs to the requester or that the value is in range.

### 12 `MSG_STONE_HINT` (server -> client), 11 bytes

Layout identical to [`MSG_SET_STONE`](#4-msg_set_stone-both-11-bytes). Carries the suggested move. Not applied to the board.

### 13 `MSG_REVOKE_PLAYER` (both), 6 bytes

| Offset | Size | Field    | Description |
|-------:|-----:|----------|-------------|
| 0      | 5    | `header` | |
| 5      | 1    | `player` | `int8`. Player slot to release. |

- Client -> server: give back a player slot. Ignored if a game is running, if `player` is outside `0..3`, or if the slot is not owned by the sender. On success the server answers the requester with `MSG_REVOKE_PLAYER` (echo), and in mode `4_COLORS_2_PLAYERS` additionally revokes `(player + 2) % 4` with a second message, then broadcasts `MSG_SERVER_STATUS`.
- Server -> client: slot taken away (also sent when a game-mode change invalidates the slot).
- The in-tree client (`CSpielClient::process_message`) has no case for `MSG_REVOKE_PLAYER` and no sender; the message is only handled by the server. Receiving it in the in-tree client logs `unbekannte Nachricht empfangen`.

### 14 `MSG_REQUEST_GAME_MODE` (client -> server), 30 bytes

Change game mode, board size and stone set. Lobby only.

| Offset | Size | Field           | Description |
|-------:|-----:|-----------------|-------------|
| 0      | 5    | `header`        | |
| 5      | 1    | `version`       | `int8`. Message version, currently `1`. Versions `< 1` are accepted but do nothing except trigger a status broadcast. |
| 6      | 1    | `width`         | `int8`. Board width, `4..50`. |
| 7      | 1    | `height`        | `int8`. Board height, `4..50`. |
| 8      | 1    | `gamemode`      | `int8`. See [game modes](#game-modes). Not range checked by the server. |
| 9      | 21   | `stone_numbers` | `int8[21]`. Copies of each stone shape per player. |

Server behaviour:

- Length `< 30`: dropped. Game running: only `MSG_SERVER_STATUS` is sent back.
- `version >= 1`: width/height out of range: dropped without answer. Otherwise applies size, mode and stone numbers, starts a fresh board, and re-seats players as needed:
  - Switching to a 2-player mode (`2_COLORS_2_PLAYERS`, `DUO`, `JUNIOR`): clients on slots `1`/`3` are revoked (`MSG_REVOKE_PLAYER`) and moved to a free slot `0`/`2` (`MSG_GRANT_PLAYER`).
  - Switching to `4_COLORS_2_PLAYERS`: each owned slot gets its partner slot `(i + 2) % 4` (`MSG_GRANT_PLAYER`) or is revoked if that slot is taken.
- Always ends with a `MSG_SERVER_STATUS` broadcast.

## Game modes

`enum GAMEMODE` in [src/constants.h](../src/constants.h), transmitted as `int8`:

| Value | Name                       |
|------:|----------------------------|
| 0     | `GAMEMODE_2_COLORS_2_PLAYERS` |
| 1     | `GAMEMODE_4_COLORS_2_PLAYERS` |
| 2     | `GAMEMODE_4_COLORS_4_PLAYERS` |
| 3     | `GAMEMODE_DUO`             |
| 4     | `GAMEMODE_JUNIOR`          |

## Typical exchanges

Join and start a 4-player game (client A is the only human):

```
A -> S   (TCP connect)
S -> A   MSG_CHAT          "Client 0 joined"
S -> A   MSG_SERVER_STATUS
A -> S   MSG_REQUEST_PLAYER  player=-1 name="A"
S -> A   MSG_GRANT_PLAYER    player=n
S -> A   MSG_SERVER_STATUS
A -> S   MSG_START_GAME
S -> A   MSG_SERVER_STATUS
S -> A   MSG_START_GAME
S -> A   MSG_CURRENT_PLAYER  player=0
```

One turn:

```
A -> S   MSG_SET_STONE       (player == current, owned by A)
S -> *   MSG_SET_STONE       (same bytes, broadcast)
S -> *   MSG_CURRENT_PLAYER  next player
S -> *   MSG_SET_STONE       (computer move, if next player is AI)
S -> *   MSG_CURRENT_PLAYER
...
S -> *   MSG_GAME_FINISH
S -> *   MSG_CURRENT_PLAYER  player=-1
```

## Worked byte example

`MSG_CURRENT_PLAYER`, `player = 0`: `data_length = 6 (0x0006)`, `msg_type = 3`.

```
check1 = (6 & 0x55) ^ 3        = 0x04 ^ 0x03 = 0x07
check2 = ((0x07 ^ 0xD6) + 3)   = 0xD1 + 3    = 0xD4

07 00 06 03 D4 00
```

## Compatibility notes

- Changing any `NET_*` struct requires version handling: `NET_SERVER_STATUS_VERSION` (3) and `NET_REQUEST_GAME_MODE_VERSION` (1). Other messages have no version field and are extended by appending fields and testing `data_length` (as `MSG_REQUEST_PLAYER` and `MSG_SERVER_STATUS` were).
- Receivers must validate `data_length` against the struct they cast onto the buffer. The server does for every client message; the client casts server messages without length checks beyond the generic header validation.
- `send()` is called once per message without a retry loop and `recv()` once per body without `MSG_WAITALL`; a message split across TCP segments is rejected as a size mismatch.
