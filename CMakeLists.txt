cmake_minimum_required(VERSION 3.14)
project(MealPrep VERSION 1.0.0 DESCRIPTION "Meal Prep - C++ meal planning tool")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- CI / Developer Build Options ---
option(ENABLE_WERROR   "Treat compiler warnings as errors"          OFF)
option(ENABLE_ASAN     "Enable AddressSanitizer + LeakSanitizer"    OFF)
option(ENABLE_UBSAN    "Enable UndefinedBehaviorSanitizer"          OFF)
option(ENABLE_TSAN     "Enable ThreadSanitizer"                     OFF)
option(ENABLE_COVERAGE "Enable code coverage instrumentation"       OFF)

if(ENABLE_TSAN AND (ENABLE_ASAN OR ENABLE_UBSAN))
  message(FATAL_ERROR "ThreadSanitizer cannot be combined with ASan or UBSan")
endif()

set(_EXTRA_COMPILE_FLAGS "")
set(_EXTRA_LINK_FLAGS "")

if(ENABLE_WERROR)
  list(APPEND _EXTRA_COMPILE_FLAGS -Wall -Wextra -Wpedantic -Werror)
endif()
if(ENABLE_ASAN)
  list(APPEND _EXTRA_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer)
  list(APPEND _EXTRA_LINK_FLAGS    -fsanitize=address)
endif()
if(ENABLE_UBSAN)
  list(APPEND _EXTRA_COMPILE_FLAGS -fsanitize=undefined)
  list(APPEND _EXTRA_LINK_FLAGS    -fsanitize=undefined)
endif()
if(ENABLE_TSAN)
  list(APPEND _EXTRA_COMPILE_FLAGS -fsanitize=thread -fno-omit-frame-pointer)
  list(APPEND _EXTRA_LINK_FLAGS    -fsanitize=thread)
endif()
if(ENABLE_COVERAGE)
  list(APPEND _EXTRA_COMPILE_FLAGS --coverage -fprofile-arcs -ftest-coverage)
  list(APPEND _EXTRA_LINK_FLAGS    --coverage)
endif()

# --- FetchContent Section ---
include(FetchContent)

# 1. Google Test
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.14.0)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
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
set(CROW_INSTALL OFF CACHE BOOL "" FORCE)
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
    src/ingredient_normalizer.cpp
    src/meal_factory.cpp
    src/meal_planner.cpp
    src/api_routes.cpp
    src/config_parser.cpp
    src/google_oauth.cpp
    src/calendar_service.cpp
    src/token_encryption.cpp
    # Add other library source files here
)

set(MAIN_SOURCE src/main.cpp)

set(TEST_SOURCES
    tests/test_ingredient.cpp
    tests/test_meal.cpp
    tests/test_meal_factory.cpp
    tests/test_measurement.cpp
    tests/test_meal_planner.cpp
    tests/test_db_manager.cpp
    tests/test_token_encryption.cpp
    tests/test_workout_service.cpp
    tests/test_calendar_service.cpp
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

# --- Apply extra flags to project targets only (not fetched deps) ---
if(_EXTRA_COMPILE_FLAGS)
  target_compile_options(meal_prep_lib  PUBLIC  ${_EXTRA_COMPILE_FLAGS})
  target_compile_options(meal_prep      PRIVATE ${_EXTRA_COMPILE_FLAGS})
  target_compile_options(meal_prep_tests PRIVATE ${_EXTRA_COMPILE_FLAGS})
endif()
if(_EXTRA_LINK_FLAGS)
  target_link_options(meal_prep_lib  PUBLIC  ${_EXTRA_LINK_FLAGS})
  target_link_options(meal_prep      PRIVATE ${_EXTRA_LINK_FLAGS})
  target_link_options(meal_prep_tests PRIVATE ${_EXTRA_LINK_FLAGS})
endif()

# --- Installation & Debian Packaging (CPack) ---
install(TARGETS meal_prep
    RUNTIME DESTINATION /opt/meal_prep/bin
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/static
    DESTINATION /opt/meal_prep
    PATTERN "mockups" EXCLUDE
)
install(FILES ${CMAKE_SOURCE_DIR}/packaging/meal_prep.conf.json
    DESTINATION /opt/meal_prep
)
if(EXISTS "${CMAKE_SOURCE_DIR}/meals.db")
    install(FILES ${CMAKE_SOURCE_DIR}/meals.db
        DESTINATION /opt/meal_prep
        RENAME meals.db.seed
        PERMISSIONS OWNER_READ OWNER_WRITE
    )
endif()

if(NOT DEFINED CALENDAR_CREDENTIALS_FILE)
    set(CALENDAR_CREDENTIALS_FILE "$ENV{HOME}/.meal_prep/dev_calendar_credentials.json")
endif()
if(EXISTS "${CALENDAR_CREDENTIALS_FILE}")
    message(STATUS "Bundling calendar credentials from ${CALENDAR_CREDENTIALS_FILE}")
    install(FILES "${CALENDAR_CREDENTIALS_FILE}"
        DESTINATION /opt/meal_prep
        RENAME calendar_credentials.json
        PERMISSIONS OWNER_READ OWNER_WRITE
    )
endif()

install(FILES ${CMAKE_SOURCE_DIR}/packaging/meal-prep.service
    DESTINATION /lib/systemd/system
)
install(FILES ${CMAKE_SOURCE_DIR}/packaging/meal-prep.default
    DESTINATION /etc/default
    RENAME meal-prep
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "meal-prep")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "JohnnyJ5")
set(CPACK_PACKAGE_CONTACT "JohnnyJ5 <https://github.com/JohnnyJ5/meal_prep>")
set(CPACK_PACKAGING_INSTALL_PREFIX "/")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "JohnnyJ5 <https://github.com/JohnnyJ5/meal_prep>")
set(CPACK_DEBIAN_PACKAGE_SECTION "web")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3, libcurl4, libsqlite3-0, ca-certificates, adduser, openssl")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_SOURCE_DIR}/packaging/postinst;${CMAKE_SOURCE_DIR}/packaging/prerm;${CMAKE_SOURCE_DIR}/packaging/conffiles"
)
include(CPack)
