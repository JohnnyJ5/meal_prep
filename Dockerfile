FROM ubuntu:22.04

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Update and install C++ dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    sudo \
    vim \
    curl \
    wget \
    python3 \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user for development
RUN useradd -m -s /bin/bash -G sudo devuser && \
    echo "devuser:devuser" | chpasswd && \
    echo "devuser ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Create work directory and set ownership
RUN mkdir -p /home/devuser/meal_prep && \
    chown -R devuser:devuser /home/devuser/meal_prep

WORKDIR /home/devuser/meal_prep

# Switch to non-root user
USER devuser

# Keep the container running in the background
CMD ["tail", "-f", "/dev/null"]