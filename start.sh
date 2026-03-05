#!/bin/bash
echo "Starting Meal Prep web server..."

# Ensure the project is built
docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && make"

# Stop any existing server instance first
./stop.sh

# Run the server in the background inside the container
docker exec -d meal_prep_dev ./build/meal_prep -s

echo "Server is running at http://localhost:8080"
echo "(It is also running on your Local IP address to access via phone)"
