🏗 Project Architecture
This project uses a Containerized Development Environment. Instead of installing C++ compilers and libraries directly on WSL/Windows, we keep everything inside a Docker container to ensure the environment is consistent and reproducible.

🚀 Quick Start
To get the development environment running:

Start the Container: ```Bash docker compose up -d```
Enter the Environment: ```Bash docker exec -it meal_prep_dev bash```

🛠 Key Components

1. The DockerfileBase Image: Ubuntu 22.04 LTS.Toolchain: build-essential (gcc/g++), cmake, and pkg-config.User: Runs as devuser (non-root) to match standard Linux permissions.Persistence: The tail -f /dev/null command keeps the container alive so you can work in it indefinitely.
2. Volumes (Code Syncing)We use a Bind Mount to sync your local files with the container:Local Path: ./ (the folder where this README is).Container Path: /home/devuser/meal_prep.Note: Any code you write in VS Code/WSL updates instantly inside the container.
3. Networking & PortsSSH: Disabled (Not needed for WSL/VS Code workflows).Future Services: If we add a database (PostgreSQL/SQLite), it will be linked via the docker-compose.yml.

💻 Common Commands
TaskCommand
Rebuild Environmentdocker compose up -d --build
Stop Environmentdocker compose down
Check Logsdocker compose logs -f
Check Statusdocker compose ps

🛠 Compilation Workflow (Internal)
Once inside the container (docker exec), use the standard CMake flow:
```Bash
# 1. Create build directory
mkdir -p build && cd build

# 2. Configure project
cmake ..

# 3. Compile
make

# 4. Run
./meal_prep_app
```
👶 Tips for the Busy DeveloperInstant Access: Add alias work='docker exec -it meal_prep_dev bash' to your ~/.bashrc.Clean Up: If you're running low on disk space, run docker system prune to clear out old build layers.