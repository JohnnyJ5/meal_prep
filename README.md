# Meal Prep Application

A C++ based meal preparation and planning application. It allows you to manage recipes, schedule meals for the week, and automatically generate consolidated grocery lists which can be emailed to you.

## Features

- **Store Meals:** Create, read, update, and delete meals and their ingredients in a local SQLite database.
- **Weekly Schedule:** Plan your meals for each day of the week.
- **Grocery List Generation:** Automatically consolidate ingredients from selected meals into a single, unified grocery list.
- **Email Notifications:** Send the final grocery list directly to your email using SMTP.
- **Web Interface:** A simple interactive web UI to plan your meals visually.
- **REST API:** A robust API backing the web interface for meal management and planning.

## Prerequisites

This project uses a fully Dockerized development environment to ensure consistency. You will need:
- Docker
- Docker Compose
- `make`

## Quickstart

All development commands are wrapped in the `Makefile` and are executed inside the Docker container automatically.

1. **Build the Environment**
   ```bash
   make build
   ```
   This command starts the background containers and compiles the C++ codebase inside the container.

2. **Start the API Server**
   ```bash
   make start
   ```
   This will start the Meal Prep API server on port 8080. You can then access the web interface at [http://localhost:8080](http://localhost:8080).

3. **Stop the Environment**
   ```bash
   make stop
   ```
   Brings down the Docker containers and cleans up the active environment.

4. **Clean Build Files**
   ```bash
   make clean
   ```
   Removes the generated build directories both natively and within the container.

## Testing

To run the automated test suite:
```bash
make test
```
This command compiles the tests and runs them using `ctest` inside the Docker environment.

## Project Structure

- `src/`: Contains all C++ source code and header files for the core backend, database management, and API routes.
- `static/`: Contains the frontend assets (HTML, CSS, JS) served by the web application.
- `tests/`: Contains the automated C++ unit tests.
- `docs/`: Contains additional project documentation, including the [API Reference](docs/API.md).
- `Dockerfile` & `docker-compose.yml`: Definitions for the Docker development environment.
- `Makefile`: Provides shortcuts for building, starting, and testing the project.
