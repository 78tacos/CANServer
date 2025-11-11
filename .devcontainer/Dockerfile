FROM debian:trixie-slim AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    g++-13 \
    gcc-13 \
    g++-13-aarch64-linux-gnu \
    gcc-13-aarch64-linux-gnu \
    can-utils \
    iproute2 \
    git \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Set g++-13 as default
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100

WORKDIR /build

COPY server.cpp Makefile ./

# Create canuser for devcontainer (with sudo access)
RUN groupadd --gid 1010 canuser \
    && useradd --uid 1010 --gid 1010 -m canuser -s /bin/bash \
    && echo canuser ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/canuser \
    && chmod 0440 /etc/sudoers.d/canuser

# Give canuser ownership of build directory
RUN chown -R canuser:canuser /build

# Switch to canuser before building to avoid root-owned artifacts
USER canuser

# Build the server statically (not that it matters)
RUN make server LDFLAGS="-static -pthread"


FROM debian:trixie-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    can-utils \
    iproute2 \
    && rm -rf /var/lib/apt/lists/*

# Create canuser with fixed UID/GID 1010
RUN groupadd --gid 1010 canuser \
    && useradd --uid 1010 --gid 1010 -m canuser

# Create necessary directories
RUN mkdir -p /app /app/logs && chown -R canuser:canuser /app

WORKDIR /app

# Copy the statically built server from builder stage
COPY --from=builder /build/output/server /app/server

# Copy example config (optional, can be overridden with volume mount)
COPY ./example_server.conf /app/server.conf

RUN chown -R canuser:canuser /app && chmod +x /app/server

USER canuser

# Port is configured via server.conf (PORT=<port>)
# Use --network=host when running the container for CAN access

ENTRYPOINT ["/app/server"]
CMD ["/app/server.conf"]