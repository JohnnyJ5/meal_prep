cmake_minimum_required(VERSION 3.14)
project(MealPrep VERSION 1.0.0 DESCRIPTION "Meal Prep - C++ meal planning tool")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# --- FetchContent Section ---
include(FetchContent)

# 1. Google Test
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.14.0)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 2. Standalone Asio (Header Only)
FetchContent_Declare(
    asio
    URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.tar.gz
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE PATH "" FORCE)

# 3. Crow (Web Framework)
FetchContent_Declare(Crow GIT_REPOSITORY https://github.com/CrowCpp/Crow.git GIT_TAG v1.2.0)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Crow)

# 2. Find system dependencies (Boost & OpenSSL)
# These must be installed in your Dockerfile (libboost-all-dev, libssl-dev)
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
# --- Source Files ---
# Explicitly listing sources is preferred over file(GLOB ...)
set(LIB_SOURCES
    src/db_manager.cpp
    src/meal_factory.cpp
    src/email_service.cpp
    src/meal_planner.cpp
    src/api_routes.cpp
    # Add other library source files here
)

set(MAIN_SOURCE src/main.cpp)

set(TEST_SOURCES
    tests/test_ingredient.cpp
    tests/test_meal.cpp
    tests/test_meal_factory.cpp
    tests/test_measurement.cpp
    tests/test_meal_planner.cpp
    # Add other test source files here
)


# --- Library Target ---
# Create a static library for the core logic
add_library(meal_prep_lib STATIC ${LIB_SOURCES})

target_include_directories(meal_prep_lib PUBLIC 
    src
    ${Boost_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(meal_prep_lib 
    PUBLIC 
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
    ${CURL_LIBRARIES}
    SQLite::SQLite3
    Crow::Crow
)

# --- Main Executable ---
add_executable(meal_prep ${MAIN_SOURCE})
target_link_libraries(meal_prep PRIVATE meal_prep_lib)


# --- Testing Section ---
enable_testing()

add_executable(meal_prep_tests ${TEST_SOURCES})
target_link_libraries(meal_prep_tests 
    PRIVATE 
    meal_prep_lib
    gtest_main
    gtest
    gmock_main
    gmock
)

include(GoogleTest)
gtest_discover_tests(meal_prep_tests)