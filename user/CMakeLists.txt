cmake_minimum_required(VERSION 3.16)
project(kfi_user VERSION 1.2.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
  message(FATAL_ERROR "KFI userspace requires Linux or Android")
endif()

include(CTest)

add_library(kfi STATIC
  libkfi/src/client.cpp
  libkfi/src/endpoint.cpp)
target_compile_features(kfi PUBLIC cxx_std_17)
target_include_directories(kfi
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libkfi/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/uapi)
target_compile_options(kfi PRIVATE -Wall -Wextra -Wpedantic)

add_executable(kfi-cli cli/main.cpp)
set_target_properties(kfi-cli PROPERTIES OUTPUT_NAME kfi)
target_link_libraries(kfi-cli PRIVATE kfi)
target_compile_options(kfi-cli PRIVATE -Wall -Wextra -Wpedantic)

if(BUILD_TESTING)
  add_executable(kfi-endpoint-test tests/endpoint_test.cpp)
  target_link_libraries(kfi-endpoint-test PRIVATE kfi)
  target_compile_options(kfi-endpoint-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME endpoint COMMAND kfi-endpoint-test)
endif()
