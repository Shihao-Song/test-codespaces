FROM ubuntu:22.04

ARG LLVM_VERSION=12

ENV DEBIAN_FRONTEND=noninteractive

# Install LLVM explicitly
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    cmake \
    ninja-build \
    python3 \
    python3-pip \
    lldb \
    llvm-${LLVM_VERSION} \
    clang-${LLVM_VERSION} \
    clangd-${LLVM_VERSION} \
    && rm -rf /var/lib/apt/lists/*

# Add LLVM to path explicitly
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 100 \
    && update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-${LLVM_VERSION} 100 \
    && update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-${LLVM_VERSION} 100

# Verify installation
RUN clang --version && llvm-config --version

WORKDIR /workspace
