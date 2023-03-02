FROM debian:bullseye AS builder
LABEL maintainer="mario.bielert@tu-dresden.de"

RUN useradd -m metricq
RUN apt-get update && apt-get install -y \
    git \
    libprotobuf-dev \
    protobuf-compiler \
    build-essential \
    cmake \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

USER metricq

COPY --chown=metricq:metricq . /home/metricq/metricq-combinator

RUN mkdir /home/metricq/metricq-combinator/build

WORKDIR /home/metricq/metricq-combinator/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && make -j 2
RUN make package

FROM debian:bullseye
LABEL maintainer="mario.bielert@tu-dresden.de"

RUN useradd -m metricq
RUN apt-get update && apt-get install -y \
    libssl1.1 \
    libprotobuf23 \
    tzdata \
    && rm -rf /var/lib/apt/lists/*

COPY --chown=metricq:metricq --from=builder /home/metricq/metricq-combinator/build/metricq-combinator-*-Linux.sh /home/metricq/metricq-combinator-Linux.sh

USER root
RUN /home/metricq/metricq-combinator-Linux.sh --skip-license --prefix=/usr

USER metricq
WORKDIR /home/metricq


ENTRYPOINT [ "/usr/bin/metricq-combinator" ]
