# --- Stage 1: Build Environment ---
FROM ubuntu:24.04 AS build-env

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    gh \
    curl \
    sudo \
    libssl-dev \
    libboost-dev \
    libcurl4-openssl-dev \
    libasio-dev \
    libsqlite3-dev \
    clang \
    clang-tidy \
    clang-format \
    && rm -rf /var/lib/apt/lists/*

# devuser for local development
RUN useradd -m -s /bin/bash -G sudo devuser && \
    echo "devuser ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /home/devuser/meal_prep
COPY . .
RUN chown -R devuser:devuser .

USER devuser
RUN make internal-build

# --- Stage 2: Production Environment ---
FROM ubuntu:24.04 AS prod-env

ENV DEBIAN_FRONTEND=noninteractive

# Only install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    libcurl4 \
    libsqlite3-0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# appuser for production (limited scope)
RUN useradd -m -s /bin/bash appuser
WORKDIR /home/appuser/app

# Copy binary from build stage
COPY --from=build-env --chown=appuser:appuser /home/devuser/meal_prep/build_docker/meal_prep .
COPY --from=build-env --chown=appuser:appuser /home/devuser/meal_prep/static ./static
COPY --from=build-env --chown=appuser:appuser /home/devuser/meal_prep/meal_prep.conf.json .

USER appuser

EXPOSE 8080

CMD ["./meal_prep", "--serve"]