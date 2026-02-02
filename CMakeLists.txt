cmake_minimum_required(VERSION 3.14)
project(MealPrep VERSION 1.0.0 DESCRIPTION "Meal Prep - C++ meal planning tool")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- CPack Configuration (Keep as is) ---
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "meal-prep")
set(CPACK_PACKAGE_VERSION "1.1.0")
set(CPACK_PACKAGE_RELEASE "1")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Meal Prep - C++ meal planning and preparation tool")
set(CPACK_PACKAGE_CONTACT "michaelcoffey5@gmail.com")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
include(CPack)

# --- FetchContent Section ---
include(FetchContent)

# 1. Google Test
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.14.0)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest )

# 2. Find system dependencies (Boost & OpenSSL)
# These must be installed in your Dockerfile (libboost-all-dev, libssl-dev)
find_package(Boost REQUIRED COMPONENTS system date_time)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(VMIME REQUIRED vmime)

# --- Main Executable ---
file(GLOB SOURCES "*.cpp")
add_executable(meal_prep ${SOURCES})

# We link Boost and Crow headers
target_include_directories(meal_prep PRIVATE 
    ${Boost_INCLUDE_DIRS}
    ${VMIME_INCLUDE_DIRS}
)

target_link_libraries(meal_prep 
    PRIVATE 
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
    Boost::system
    Boost::date_time
    ${VMIME_LIBRARIES}
)

# --- Testing Section ---
enable_testing()
file(GLOB TEST_SOURCES "tests/*.cpp")
file(GLOB LIB_SOURCES "*.cpp")
list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_executable(meal_prep_tests ${TEST_SOURCES} ${LIB_SOURCES})
target_include_directories(meal_prep_tests PRIVATE  ${Boost_INCLUDE_DIRS} ${VMIME_INCLUDE_DIRS})
target_link_libraries(meal_prep_tests 
    PRIVATE 
    gtest_main
    gtest
    gmock_main
    gmock
    Boost::system
    Boost::date_time
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
    ${VMIME_LIBRARIES}
)

include(GoogleTest)
gtest_discover_tests(meal_prep_tests)