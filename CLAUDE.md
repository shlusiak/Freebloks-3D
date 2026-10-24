# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

Generated autotools files (configure, Makefile.in, aclocal.m4, config.h) are gitignored. Bootstrap from a clean checkout:

    aclocal; autoreconf -i; automake --add-missing
    ./configure --without-gui --disable-dependency-tracking
    make

configure options: --without-gui, --without-dedicated, --without-client, --without-benchmark. The GUI needs X11, GL, GLU, Xpm. C++ is gnu++17 -O3 (src/Makefile.am).

Binaries: freebloks (OpenGL/X11 GUI), freebloks_dedicated (headless server), freebloks_client (console client), freebloks_benchmark (AI benchmark; spins up server + clients). There is no test suite or make check; the benchmark is the closest thing.

Docker: Dockerfile builds --without-gui on Alpine. docker-compose.yml runs server + Prometheus + Grafana (deploy/ holds provisioning). Container config via env LOGFILE, LIMIT, METRICS_PORT, METRICS_LISTEN.

Windows (.sln/.vcproj/.dsp, config-win32.h), debian/, build/, *.ebuild and .deb files are legacy and not maintained alongside src/Makefile.am (e.g. telemetry.cpp is absent from them).

## Architecture

Flat C++ code base in src/, C-prefixed classes. Some files are Latin-1 encoded; do not re-encode.

Language policy: all documentation, comments, identifiers and file names are English. German is legacy and is being removed. Write new code, comments, docs and file names in English only, and translate or rename German ones when touching them. File names and identifiers are already English; comments are still largely German. Glossary for reading them: Spiel=game, Spieler=player, Stein=stone, Zug=turn, KI=AI. Renaming a NET_* field does not change the wire format; renaming files requires updating src/Makefile.am.

- Core: CBoard (grid + rules) -> CGame (state, current player, game mode, turn history). player[] holds a socket fd or PLAYER_COMPUTER (-2) / PLAYER_LOCAL (-1). Game modes in constants.h.
- AI: CAI (ai.cpp), strength AI_* and thread count configurable.
- Protocol (network.h): TCP port 59995, #pragma pack(1) structs cast directly onto the buffer. Every message starts with NET_HEADER {check1, data_length (network byte order), msg_type, check2}. data_length must be validated on receive. Changing a NET_* struct needs version handling (NET_SERVER_STATUS_VERSION, NET_REQUEST_GAME_MODE_VERSION).
- Server: CGameServer (gameserver.cpp) is authoritative; process_message() is the central message switch. CServerListener handles up to 8 listen sockets.
- Clients: CGameClient (network), CGUIGameClient (GUI hooks), client.cpp (console).
- GUI (freebloks binary only): freebloks.cpp, window.cpp (X11/GLX), gui.cpp, widgets/dialogs; textures installed to $(datadir)/freebloks, DATA_PREFIX set via freebloks_CXXFLAGS.
- Dedicated server (dedicated.cpp): establish_game() accepts clients, gameRunThread runs each game on a detached thread, --limit caps concurrent games (listener closes at limit, re-opens when below). --stats reads an abstract unix socket (/tmp/freebloks_stats).
- Telemetry (telemetry.cpp/.h, namespace telemetry): lock-free atomic counters/gauges/histograms exposed as Prometheus text on GET /metrics and /healthz by a detached HTTP thread. Off unless --metrics-port is non-zero; binds 127.0.0.1 by default. Hooks live in gameserver.cpp and dedicated.cpp. Linked into dedicated, benchmark and GUI sources, not freebloks_client. POSIX-only.
- Logger (logger.cpp): process-wide recursive mutex; always call logLine("%s", s), never pass untrusted strings as the format.

Documentation is in docs/ (English). docs/protocol.md is the wire-format reference; update it whenever network.h or message handling changes.
