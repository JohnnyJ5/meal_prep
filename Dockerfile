FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Added libasio-dev for networking
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libboost-all-dev \
    libvmime-dev \
    sudo \
    curl \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash -G sudo devuser && \
    echo "devuser:devuser" | chpasswd && \
    echo "devuser ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Create work directory
WORKDIR /home/devuser/meal_prep
RUN chown -R devuser:devuser /home/devuser/meal_prep

USER devuser

# Expose the port your server will listen on
EXPOSE 8080

CMD ["tail", "-f", "/dev/null"]