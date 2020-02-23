################## BUILDER
FROM alpine:latest AS builder

RUN apk add autoconf automake g++ make
# RUN apk add mesa-dev glu-dev

ADD . /install
WORKDIR /install

RUN aclocal && autoconf && automake --add-missing
RUN ./configure --without-gui --disable-dependency-tracking
RUN make clean && make all -j 4

RUN strip src/freebloks_client src/freebloks_dedicated src/freebloks_benchmark


################## IMAGE
FROM alpine:latest

ENV LOGFILE=/install/freebloks_dedicated.log
ENV LIMIT=15

RUN apk add libgcc libstdc++

WORKDIR /install
COPY --from=builder /install/src/freebloks_* ./

ENTRYPOINT /install/freebloks_dedicated --log $LOGFILE --limit $LIMIT