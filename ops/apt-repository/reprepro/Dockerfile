FROM ubuntu:24.04 AS build

ARG DEBIAN_FRONTEND=noninteractive
ARG REPREPRO_VERSION=5.5.1
ARG REPREPRO_TAG=reprepro-5.5.1
ARG REPREPRO_COMMIT=2833265135bff5fc5b01c8e256cf9b46d602a59c

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      autoconf automake build-essential ca-certificates git libarchive-dev libbz2-dev libdb-dev \
      libgpgme-dev liblzma-dev libzstd-dev pkg-config \
 && rm -rf /var/lib/apt/lists/*

RUN git clone --branch "$REPREPRO_TAG" --depth 1 \
      https://salsa.debian.org/debian/reprepro.git /src/reprepro \
 && test "$(git -C /src/reprepro rev-parse HEAD)" = "$REPREPRO_COMMIT" \
 && cd /src/reprepro \
 && ./autogen.sh \
 && ./configure --prefix=/opt/winegui-reprepro \
 && make -C /src/reprepro -j"$(nproc)" \
 && install -D -m 0755 /src/reprepro/reprepro /out/bin/reprepro \
 && test "$(/out/bin/reprepro --version 2>&1 | sed -n 's/^.*: This is reprepro version //p')" = "$REPREPRO_VERSION" \
 && printf '%s\n' "$REPREPRO_VERSION" > /out/REPREPRO_VERSION \
 && printf '%s\n' "$REPREPRO_COMMIT" > /out/REPREPRO_COMMIT

FROM ubuntu:24.04
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates libarchive13t64 libbz2-1.0 libdb5.3t64 libgpgme11t64 liblzma5 libzstd1 \
 && rm -rf /var/lib/apt/lists/*
COPY --from=build /out /opt/winegui-reprepro
ENTRYPOINT ["/opt/winegui-reprepro/bin/reprepro"]
