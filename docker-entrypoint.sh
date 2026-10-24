#!/bin/sh
# Logs go to stdout for the container runtime; --log is only added when LOGFILE is set.
set -- --limit "$LIMIT"
[ -n "$PORT" ] && set -- "$@" --port "$PORT"
[ -n "$LISTEN" ] && set -- "$@" -L "$LISTEN"
[ -n "$MODE" ] && set -- "$@" --mode "$MODE"
[ -n "$AI" ] && set -- "$@" --ai "$AI"
[ -n "$WIDTH" ] && set -- "$@" --width "$WIDTH"
[ -n "$HEIGHT" ] && set -- "$@" --height "$HEIGHT"
[ -n "$MAX_HUMANS" ] && set -- "$@" --maxhumans "$MAX_HUMANS"
[ -n "$THREADS" ] && set -- "$@" --threads "$THREADS"
[ -n "$LOGFILE" ] && set -- "$@" --log "$LOGFILE"
exec /install/freebloks_dedicated "$@"
