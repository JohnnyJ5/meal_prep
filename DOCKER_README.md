🏗 Project Architecture

This project uses a Containerized Development Environment. Instead of installing C++ compilers and libraries directly on WSL/Windows, we keep everything inside a Docker container to ensure the environment is consistent and reproducible.


🚀 Quick Start
To get the development environment running:

Start the Container: 
```bash
docker compose up -d
```

Enter the Environment: 
```bash
docker exec -it meal_prep_dev bash
```


🛠 Key Components
1. The Dockerfile
    * Base Image: Ubuntu 22.04 LTS.
    * Toolchain: build-essential (gcc/g++), cmake, and pkg-config.
    * User: Runs as devuser (non-root) to match standard Linux permissions.
    * Persistence: The tail -f /dev/null command keeps the container alive so you can work in it indefinitely.
2. Volumes (Code Syncing)
We use a Bind Mount to sync your local files with the container:
    * Local Path: ./ (the folder where this README is).
    * Container Path: /home/devuser/meal_prep.
      > **Note:** Any code you write in VS Code/WSL updates instantly inside the container.
3. Networking & PortsSSH: Disabled (Not needed for WSL/VS Code workflows).Future Services: If we add a database (PostgreSQL/SQLite), it will be linked via the docker-compose.yml.


💻 Common Commands
| Task | Command |
| :--- | :--- |
| Rebuild Environment | `docker compose up -d --build`  |
| Stop Environment | `docker compose down` |
| Check Logs | `docker compose logs -f`|
| Check Status | `docker compose ps` |


🛠 Compilation & Running Workflow
We have provided a helper script that automatically builds and runs the application from the correct directory:

```bash
docker exec -it meal_prep_dev ./start_server.sh
```

*(Note: The server must be started from the project root so it can find the `static/` directory! The script handles this automatically.)*

Alternatively, if you want to compile and run it manually inside the container:
```bash
# 1. Enter the container
docker exec -it meal_prep_dev bash

# 2. Create build directory
mkdir -p build && cd build

# 3. Configure and Compile
cmake ..
make -j

# 4. Return to root and Run
cd ..
./build/meal_prep --serve
```

👶 Tips for the Busy Developer
- **Instant Access:** Add `alias work='docker exec -it meal_prep_dev bash'` to your `~/.bashrc`.
- **Instant Server:** Add `alias start_meal_server='docker exec -it meal_prep_dev ./start_server.sh'` to your `~/.bashrc`.
- **Clean Up:** If you're running low on disk space, run `docker system prune` to clear out old build layers.
