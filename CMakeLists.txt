cmake_minimum_required(VERSION 3.14)
project(MealPrep)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main executable
file(GLOB SOURCES "*.cpp")
add_executable(meal_prep ${SOURCES})
# Find OpenSSL for SSL symbols used by mailio/boost::asio
find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
  target_link_libraries(meal_prep PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# Try to locate mailio headers/library. If not found, emit a helpful warning
find_path(MAILIO_INCLUDE_DIR NAMES mailio/message.hpp)
find_library(MAILIO_LIBRARY NAMES mailio)
if(MAILIO_INCLUDE_DIR AND MAILIO_LIBRARY)
  target_include_directories(meal_prep PRIVATE ${MAILIO_INCLUDE_DIR})
  target_link_libraries(meal_prep PRIVATE ${MAILIO_LIBRARY})
else()
  message(WARNING "mailio headers or library not found. Install mailio or adjust CMake to link it. This will cause linker errors for mailio symbols.")
endif()
# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test executable
enable_testing()

file(GLOB TEST_SOURCES "tests/*.cpp")
# Create test sources excluding main.cpp
file(GLOB LIB_SOURCES "*.cpp")
list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
add_executable(meal_prep_tests ${TEST_SOURCES} ${LIB_SOURCES})
target_link_libraries(meal_prep_tests 
    PRIVATE 
    gtest_main
    gtest
    gmock_main
    gmock
)

include(GoogleTest)
gtest_discover_tests(meal_prep_tests)