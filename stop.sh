#!/bin/bash
echo "Stopping Meal Prep web server..."
docker exec meal_prep_dev pkill -f meal_prep || true
echo "Server stopped."
