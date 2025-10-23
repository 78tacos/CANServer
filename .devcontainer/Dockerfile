# Use an official base image
FROM ubuntu:24.04

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install essential build tools and dependencies for native and ARM64 cross-compilation
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
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
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Set the working directory
WORKDIR /workspace

# Change ownership of the workspace
RUN chown -R $USERNAME:$USERNAME /workspace

# Switch to non-root user
USER $USERNAME

# Set the default shell
CMD ["/bin/bash"]