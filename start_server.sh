#!/bin/bash
set -e

# Ensure we are always executing from the directory where this script lives (the project root)
cd "$(dirname "$0")"

echo "Building the project..."
mkdir -p build
cd build
cmake ..
make -j

# Return to the project root
cd ..

echo "Starting Meal Prep API on port 8080..."
# Execute from the project root so it can find the static/ folder!
./build/meal_prep --serve &
