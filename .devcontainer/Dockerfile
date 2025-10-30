FROM debian:trixie-slim AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY server.cpp Makefile ./

# Build the server statically (not that it matters)
RUN make server LDFLAGS="-static -pthread"

FROM debian:trixie-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    can-utils \
    iproute2 \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user
ARG USERNAME=canuser
ARG USER_UID=1010
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME

# Create necessary directories
RUN mkdir -p /app /app/logs && chown -R $USERNAME:$USERNAME /app

WORKDIR /app

# Copy the statically built server from builder stage
COPY --from=builder /build/output/server /app/server

# Copy example config (optional, can be overridden with volume mount)
COPY output/example_server.conf /app/server.conf

RUN chown -R $USERNAME:$USERNAME /app && chmod +x /app/server

USER $USERNAME

# Port is configured via server.conf (PORT=<port>)
# Use --network=host when running the container for CAN access

ENTRYPOINT ["/app/server"]
CMD ["/app/server.conf"]