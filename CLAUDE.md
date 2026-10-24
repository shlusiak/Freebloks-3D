# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

Generated autotools files (configure, Makefile.in, aclocal.m4, config.h) are gitignored. Bootstrap from a clean checkout:

    aclocal; autoreconf -i; automake --add-missing
    ./configure --without-gui --disable-dependency-tracking
    make

configure options: --without-gui, --without-dedicated, --without-client, --without-benchmark. The GUI needs X11, GL, GLU. C++ is gnu++17 -O3 (src/Makefile.am).

Binaries: freebloks (OpenGL/X11 GUI), freebloks_dedicated (headless server), freebloks_client (console client), freebloks_benchmark (AI benchmark; spins up server + clients). There is no test suite or make check; the benchmark is the closest thing.

Docker: Dockerfile builds --without-gui on Alpine and runs freebloks_dedicated. Container config via env LOGFILE, LIMIT.

Windows (.sln/.vcproj/.dsp, config-win32.h), debian/, build/, *.ebuild and .deb files are legacy and not maintained alongside src/Makefile.am.

## Android consumer

The server portion of this repository (game logic, AI, networking, CGameServer) is used by the Android project in a JNI module. The Android project includes this repository as a git submodule and compiles the sources itself with a custom config.h, not through autotools or src/Makefile.am. Consequences:

- Server-side sources (everything except the GUI, client.cpp, dedicated.cpp and benchmark.cpp) must compile without the autotools-generated config.h; do not rely on HAVE_* macros or other configure results being defined.
- The Android project pins a specific commit of this repository, so changes here do not break its existing build. Breaking changes (including adding, renaming or removing server-side source files, since Android keeps its own file list) are acceptable, but refactoring must keep the server portion compilable for Android with a custom config.h once the Android project is updated to the new commit.
- Keep server-side code free of X11, GL and other GUI dependencies.

## Architecture

Flat C++ code base in src/, C-prefixed classes.

Language policy: all documentation, comments, identifiers and file names are English. Write new code, comments, docs and file names in English only. Sources are plain ASCII. Renaming a NET_* field does not change the wire format; renaming files requires updating src/Makefile.am.

- Core: CBoard (grid + rules) -> CGame (state, current player, game mode, turn history). player[] holds a socket fd or PLAYER_COMPUTER (-2) / PLAYER_LOCAL (-1). Game modes in constants.h.
- AI: CAI (ai.cpp), strength AI_* and thread count configurable.
- Protocol (network.h): TCP port 59995 (port 0 = local Unix domain socket: abstract namespace on Linux, socket file elsewhere, via local_socket_address() in network.cpp), #pragma pack(1) structs cast directly onto the buffer. Every message starts with NET_HEADER {check1, data_length (network byte order), msg_type, check2}. data_length must be validated on receive. Changing a NET_* struct needs version handling (NET_SERVER_STATUS_VERSION, NET_REQUEST_GAME_MODE_VERSION).
- Server: CGameServer (gameserver.cpp) is authoritative; process_message() is the central message switch. CServerListener handles up to 8 listen sockets.
- Clients: CGameClient (network), CGUIGameClient (GUI hooks), client.cpp (console).
- GUI (freebloks binary only): freebloks.cpp, window.cpp (X11/GLX), gui.cpp, widgets/dialogs; textures installed to $(datadir)/freebloks, DATA_PREFIX set via freebloks_CXXFLAGS.
- Dedicated server (dedicated.cpp): establish_game() accepts clients, gameRunThread runs each game on a detached thread, --limit caps concurrent games (listener closes at limit, re-opens when below). A stats thread serves counters on the Unix socket file /tmp/freebloks_stats; --stats connects to it, prints them and exits.
- Logger (logger.cpp): emits one logfmt line per call (docs/logging.md), serialized by a process-wide mutex. Use log()/logClient()/logFields() with a LogLevel; never pass untrusted strings as the format (use "%s"). The fields argument of logFields() is copied verbatim and must be trusted. Server-side code logs through CLogger only, not printf/perror. dedicated takes --log <file> (Docker: LOGFILE env is passed as --log).

Documentation is in docs/ (English). docs/protocol.md is the wire-format reference; update it whenever network.h or message handling changes.
