.PHONY: all build start stop test clean internal-build lint lint-fix \
        asan tsan coverage cppcheck

all: build

build:
	@echo "Ensuring Docker environment is up (Development stage)..."
	docker compose build meal_prep_dev
	docker compose up -d meal_prep_dev
	@echo "Building the project inside the container..."
	docker exec meal_prep_dev make internal-build

internal-build:
	@echo "Running internal build process..."
	mkdir -p build_docker && cd build_docker && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. && make -j$(nproc)

start: build
	@echo "Stopping any existing server instances..."
	-docker exec meal_prep_dev pkill meal_prep || true
	@echo "Starting Meal Prep API on port 8080 inside container..."
	docker exec -d meal_prep_dev bash -c "cd /home/devuser/meal_prep && ./build_docker/meal_prep --serve > server.log 2>&1"

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

lint: build
	@echo "Running lintenator (clang-tidy + clang-format) inside container..."
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && ./scripts/lintenator.sh --build-dir build_docker"

lint-fix: build
	@echo "Running lintenator with --fix inside container..."
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && ./scripts/lintenator.sh --build-dir build_docker --fix"

asan: build
	@echo "Building with AddressSanitizer + UBSan inside container..."
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && \
	  mkdir -p build_asan && cd build_asan && \
	  cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON .. && \
	  make -j\$$(nproc) && \
	  ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 \
	  UBSAN_OPTIONS=print_stacktrace=1:abort_on_error=1 \
	  ctest --output-on-failure"

tsan: build
	@echo "Building with ThreadSanitizer inside container..."
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && \
	  mkdir -p build_tsan && cd build_tsan && \
	  cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON .. && \
	  make -j\$$(nproc) && \
	  TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 \
	  ctest --output-on-failure"

coverage: build
	@echo "Building with coverage instrumentation inside container..."
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && \
	  mkdir -p build_cov && cd build_cov && \
	  cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON .. && \
	  make -j\$$(nproc) && \
	  ctest --output-on-failure && \
	  lcov --capture --directory . --output-file coverage_raw.info && \
	  lcov --remove coverage_raw.info '/usr/*' '*/build_cov/_deps/*' '*/tests/*' \
	       --output-file coverage.info && \
	  lcov --summary coverage.info"

cppcheck:
	@echo "Running cppcheck inside container..."
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && \
	  cppcheck --error-exitcode=1 --enable=warning,style,performance,portability \
	    --suppress=missingIncludeSystem --suppress=missingInclude \
	    --inline-suppr --std=c++17 --check-level=exhaustive \
	    -I src src/ tests/"