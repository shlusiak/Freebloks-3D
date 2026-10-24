################## BUILDER
FROM alpine:3.20 AS builder

RUN apk add autoconf automake g++ make

COPY . /install
WORKDIR /install

RUN aclocal \
 && autoreconf -i \
 && automake --add-missing \
 && ./configure --without-gui --without-client --without-benchmark --disable-dependency-tracking \
 && make clean && make all -j"$(nproc)" \
 && strip src/freebloks_dedicated


################## IMAGE
FROM alpine:3.20

LABEL org.opencontainers.image.title="freebloks_dedicated" \
      org.opencontainers.image.description="Dedicated Freebloks game server" \
      org.opencontainers.image.licenses="GPL-2.0-or-later" \
      org.opencontainers.image.source="https://github.com/shlusiak/Freebloks"

ENV LIMIT=15
# Optional overrides, unset by default: PORT LISTEN MODE AI WIDTH HEIGHT MAX_HUMANS THREADS LOGFILE

RUN apk add --no-cache libgcc libstdc++ \
 && addgroup -g 10001 -S freebloks \
 && adduser -u 10001 -S -G freebloks -H -s /sbin/nologin freebloks

WORKDIR /install
COPY --from=builder /install/src/freebloks_dedicated ./
COPY docker-entrypoint.sh .

USER freebloks:freebloks

# Documentation only; the compose file uses host networking
EXPOSE 59995

# --stats exits non-zero if the stats socket of the running server is unreachable
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s \
  CMD /install/freebloks_dedicated --stats >/dev/null || exit 1

ENTRYPOINT ["/install/docker-entrypoint.sh"]
