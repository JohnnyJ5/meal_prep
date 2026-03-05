#!/bin/bash
echo "Starting Meal Prep web server..."

# Ensure the project is built
docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && make"

# Stop any existing server instance first
./stop.sh

# Run the server in the background inside the container
docker exec -d meal_prep_dev ./build/meal_prep -s

# Fetch the current LAN IP address and print it clearly
LAN_IP=$(ip -4 addr show scope global | grep -v 'docker\|br-\|veth' | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n 1)

echo "------------------------------------------------------"
echo "✅ Server is running at http://localhost:8080"
echo "📱 Access from your phone at: http://${LAN_IP}:8080"
echo "------------------------------------------------------"
