.PHONY: all build start stop test clean internal-build lint lint-fix \
        asan tsan coverage cppcheck

LOCAL ?= 0

# Auto-enable LOCAL mode when running as johnnyj or devuser
ifneq ($(filter $(shell whoami),johnnyj devuser),)
  LOCAL := 1
endif

all: build

build:
ifeq ($(LOCAL),1)
	@echo "Building locally..."
	mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. && make -j$(nproc)
else
	@echo "Ensuring Docker environment is up (Development stage)..."
	docker compose build meal_prep_dev
	docker compose up -d meal_prep_dev
	@echo "Building the project inside the container..."
	docker exec meal_prep_dev make internal-build
endif

internal-build:
	@echo "Running internal build process..."
	mkdir -p build_docker && cd build_docker && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. && make -j$(nproc)

start: build
ifeq ($(LOCAL),1)
	@echo "Stopping any existing server instances..."
	-pkill meal_prep || true
	@echo "Starting Meal Prep API on port 8080 locally..."
	./build/meal_prep --serve > server.log 2>&1 &
else
	@echo "Stopping any existing server instances..."
	-docker exec meal_prep_dev pkill meal_prep || true
	@echo "Starting Meal Prep API on port 8080 inside container..."
	docker exec -d meal_prep_dev bash -c "cd /home/devuser/meal_prep && ./build_docker/meal_prep --serve > server.log 2>&1"
endif

stop:
ifeq ($(LOCAL),1)
	@echo "Stopping local server..."
	-pkill meal_prep || true
	@echo "Server stopped."
else
	@echo "Stopping Meal Prep environment..."
	docker compose down
	@echo "Environment stopped."
endif

test: build
ifeq ($(LOCAL),1)
	@echo "Running tests locally..."
	cd build && ctest --output-on-failure
else
	@echo "Running tests inside the container..."
	docker exec meal_prep_dev bash -c "cd build_docker && ctest --output-on-failure"
endif

clean:
ifeq ($(LOCAL),1)
	@echo "Cleaning local build directory..."
	rm -rf ./build
else
	$(MAKE) stop
	@echo "Cleaning build directories..."
	rm -rf ./build ./build_docker
endif

lint: build
ifeq ($(LOCAL),1)
	@echo "Running clang-tidy locally..."
	./scripts/clang-tidy.sh --build-dir build
	@echo "Running clang-format locally..."
	./scripts/clang-format.sh
else
	@echo "Running clang-tidy inside container..."
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && ./scripts/clang-tidy.sh --build-dir build_docker"
	@echo "Running clang-format inside container..."
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && ./scripts/clang-format.sh"
endif

lint-fix: build
ifeq ($(LOCAL),1)
	@echo "Running clang-tidy with --fix locally..."
	./scripts/clang-tidy.sh --build-dir build --fix
	@echo "Running clang-format with --fix locally..."
	./scripts/clang-format.sh --fix
else
	@echo "Running clang-tidy with --fix inside container..."
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && ./scripts/clang-tidy.sh --build-dir build_docker --fix"
	@echo "Running clang-format with --fix inside container..."
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && ./scripts/clang-format.sh --fix"
endif

asan:
ifeq ($(LOCAL),1)
	@echo "Building with AddressSanitizer + UBSan locally..."
	mkdir -p build_asan && cd build_asan && \
	  cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON .. && \
	  make -j$(nproc) && \
	  ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 \
	  UBSAN_OPTIONS=print_stacktrace=1:abort_on_error=1 \
	  ctest --output-on-failure
else
	@echo "Building with AddressSanitizer + UBSan inside container..."
	docker compose up -d meal_prep_dev
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && \
	  mkdir -p build_asan && cd build_asan && \
	  cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON .. && \
	  make -j\$$(nproc) && \
	  ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 \
	  UBSAN_OPTIONS=print_stacktrace=1:abort_on_error=1 \
	  ctest --output-on-failure"
endif

tsan:
ifeq ($(LOCAL),1)
	@echo "Building with ThreadSanitizer locally..."
	mkdir -p build_tsan && cd build_tsan && \
	  cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON .. && \
	  make -j$(nproc) && \
	  TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 \
	  ctest --output-on-failure
else
	@echo "Building with ThreadSanitizer inside container..."
	docker compose up -d meal_prep_dev
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && \
	  mkdir -p build_tsan && cd build_tsan && \
	  cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON .. && \
	  make -j\$$(nproc) && \
	  TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 \
	  ctest --output-on-failure"
endif

coverage:
ifeq ($(LOCAL),1)
	@echo "Building with coverage instrumentation locally..."
	mkdir -p build_cov && cd build_cov && \
	  cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON .. && \
	  make -j$(nproc) && \
	  ctest --output-on-failure && \
	  lcov --capture --directory . --output-file coverage_raw.info && \
	  lcov --remove coverage_raw.info '/usr/*' '*/build_cov/_deps/*' '*/tests/*' \
	       --output-file coverage.info && \
	  lcov --summary coverage.info
else
	@echo "Building with coverage instrumentation inside container..."
	docker compose up -d meal_prep_dev
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && \
	  mkdir -p build_cov && cd build_cov && \
	  cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON .. && \
	  make -j\$$(nproc) && \
	  ctest --output-on-failure && \
	  lcov --capture --directory . --output-file coverage_raw.info && \
	  lcov --remove coverage_raw.info '/usr/*' '*/build_cov/_deps/*' '*/tests/*' \
	       --output-file coverage.info && \
	  lcov --summary coverage.info"
endif

cppcheck:
ifeq ($(LOCAL),1)
	@echo "Running cppcheck locally..."
	cppcheck --error-exitcode=1 --enable=warning,style,performance,portability \
	  --suppress=missingIncludeSystem --suppress=missingInclude \
	  --inline-suppr --std=c++17 \
	  -I src src/ tests/
else
	@echo "Running cppcheck inside container..."
	docker compose up -d meal_prep_dev
	docker exec meal_prep_dev bash -c "cd /home/devuser/meal_prep && \
	  cppcheck --error-exitcode=1 --enable=warning,style,performance,portability \
	    --suppress=missingIncludeSystem --suppress=missingInclude \
	    --inline-suppr --std=c++17 --check-level=exhaustive \
	    -I src src/ tests/"
endif