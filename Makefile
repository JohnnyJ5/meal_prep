.PHONY: all build start stop test clean

all: build

build:
	@echo "Ensuring Docker environment is up..."
	docker compose up -d
	@echo "Building the project inside the container..."
	docker exec meal_prep_dev bash -c "mkdir -p build_docker && cd build_docker && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. && make -j"

start: build
	@echo "Starting Meal Prep API on port 8080 inside container..."
	docker exec -d meal_prep_dev bash -c "cd /home/devuser/meal_prep && ./build_docker/meal_prep --serve"

stop:
	@echo "Stopping Meal Prep environment..."
	docker compose down
	@echo "Environment stopped."

test: build
	@echo "Running tests inside the container..."
	docker exec meal_prep_dev bash -c "cd build_docker && ctest --output-on-failure"

clean: stop
	@echo "Cleaning build directories natively to ensure complete reset..."
	rm -rf ./build ./build_docker