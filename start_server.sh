#!/bin/bash
set -e

# Ensure we are always executing from the directory where this script lives (the project root)
cd "$(dirname "$0")"

echo "Starting Docker environment..."
docker compose up -d

echo "Building the project inside the container..."
docker exec meal_prep_dev bash -c "mkdir -p build_docker && cd build_docker && cmake .. && make -j"

echo "Starting Meal Prep API on port 8080 inside container..."
# Execute from the project root inside container so it can find the static/ folder!
docker exec -d meal_prep_dev bash -c "cd /home/devuser/meal_prep && ./build_docker/meal_prep --serve"
