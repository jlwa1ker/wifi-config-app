cmake_minimum_required(VERSION 3.14)
project(wifi-config-app-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# Fetch RapidCheck
FetchContent_Declare(
  rapidcheck
  GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
  GIT_TAG master
)
FetchContent_MakeAvailable(rapidcheck)

# Fetch ArduinoJson (header-only, works on any platform)
FetchContent_Declare(
  ArduinoJson
  GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
  GIT_TAG v6.21.5
)
FetchContent_MakeAvailable(ArduinoJson)

# Include mock headers and project source
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Enable CTest and Catch2 test discovery
include(CTest)
include(Catch)

# Property test: credential round-trip (Property 1)
add_executable(test_credential_store test_credential_store.cpp)
target_link_libraries(test_credential_store PRIVATE Catch2::Catch2WithMain rapidcheck)
catch_discover_tests(test_credential_store)

# Property test: failed credentials not persisted (Property 2)
add_executable(test_credential_store_no_persist test_credential_store_no_persist.cpp)
target_link_libraries(test_credential_store_no_persist PRIVATE Catch2::Catch2WithMain rapidcheck)
catch_discover_tests(test_credential_store_no_persist)

# Property test: retry logic bounds (Property 8)
add_executable(test_health_retry test_health_retry.cpp)
target_link_libraries(test_health_retry PRIVATE Catch2::Catch2WithMain rapidcheck)
catch_discover_tests(test_health_retry)

# Property test: health cache behavior (Property 6)
add_executable(test_health_cache test_health_cache.cpp)
target_link_libraries(test_health_cache PRIVATE Catch2::Catch2WithMain rapidcheck)
catch_discover_tests(test_health_cache)

# Property test: JSON health response parsing (Property 3)
add_executable(test_health_parser test_health_parser.cpp)
target_include_directories(test_health_parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_health_parser PRIVATE Catch2::Catch2WithMain rapidcheck ArduinoJson)
catch_discover_tests(test_health_parser)

# Unit test: URL decoding (Requirement 3.1)
add_executable(test_url_decode test_url_decode.cpp)
target_include_directories(test_url_decode PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_url_decode PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(test_url_decode)

# Property test: landing page HTML content (Property 7)
add_executable(test_landing_page test_landing_page.cpp)
target_include_directories(test_landing_page PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_landing_page PRIVATE Catch2::Catch2WithMain rapidcheck)
catch_discover_tests(test_landing_page)
