FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    git \
    curl \
    vim \
    nano \
    sudo \
    ca-certificates \
    lsb-release \
    pkg-config \
    gdb \
    strace \
    can-utils \
    iproute2 \
    kmod \
    libcap2-bin \
# Install newer GCC/G++ (provide C++20 support) and matching cross-compilers
    g++-13 \
    gcc-13 \
    g++-13-aarch64-linux-gnu \
    gcc-13-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user for development
ARG USERNAME=vscode
ARG USER_UID=1010
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

WORKDIR /workspace

RUN chown -R $USERNAME:$USERNAME /workspace

USER $USERNAME

CMD ["/bin/bash"]