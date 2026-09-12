FROM ubuntu:24.04 AS build

ARG DEBIAN_FRONTEND=noninteractive
ARG REPREPRO_COMMIT=3afde91f87342b473bb624f3bf3c5cc0341b75e8

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential ca-certificates git libarchive-dev libbz2-dev libdb-dev \
      libgpgme-dev liblzma-dev libzstd-dev pkg-config \
 && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/ionos-cloud/reprepro.git /src/reprepro \
 && git -C /src/reprepro checkout --detach "$REPREPRO_COMMIT" \
 && test "$(git -C /src/reprepro rev-parse HEAD)" = "$REPREPRO_COMMIT" \
 && cd /src/reprepro \
 && ./configure --prefix=/opt/winegui-reprepro \
 && make -C /src/reprepro -j"$(nproc)" \
 && install -D -m 0755 /src/reprepro/reprepro /out/bin/reprepro \
 && printf '%s\n' "$REPREPRO_COMMIT" > /out/REPREPRO_COMMIT

FROM ubuntu:24.04
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates libarchive13t64 libbz2-1.0 libdb5.3t64 libgpgme11t64 liblzma5 libzstd1 \
 && rm -rf /var/lib/apt/lists/*
COPY --from=build /out /opt/winegui-reprepro
ENTRYPOINT ["/opt/winegui-reprepro/bin/reprepro"]
