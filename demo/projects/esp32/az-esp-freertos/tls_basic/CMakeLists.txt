# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common
                         ../)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tls_basic)

message(STATUS "project register")
message(STATUS " dirs: project-${PROJECT_DIR} cmake_source-${CMAKE_SOURCE_DIR}")
target_add_binary_data(tls_basic.elf "../certs/${CONFIG_IOTHUB_ROOTCA_PEM}" TEXT)
