# Docker & Development Commands

This guide provides essential Docker-related commands for local development of the Meal Prep application.

## Primary Workflow (Makefile)

Most commands are wrapped in the `Makefile` and should be run using `make`.

| Command | Description |
| :--- | :--- |
| `make build` | Builds the Docker environment and compiles the C++ code. |
| `make start` | Starts the API server (available at http://localhost:8080). |
| `make stop` | Stops and removes the Docker containers. |
| `make test` | Runs the automated test suite inside the container. |
| `make clean` | Removes build artifacts and temporary files. |

## manual Docker Commands

If you need to interact with Docker directly:

### View Running Containers
```bash
docker ps
```

### View Container Logs
```bash
docker-compose logs -f app
```

### Execute Shell inside Container
```bash
docker-compose exec app /bin/bash
```

### Rebuild without Cache
```bash
docker-compose build --no-cache
```

### Clean up Volumes and Orphanded Images
```bash
docker system prune -a --volumes
```
