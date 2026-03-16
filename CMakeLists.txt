cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
project(JANA2_NPS_ONNX LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

if(NOT "${CMAKE_CXX_STANDARD}")
    set(CMAKE_CXX_STANDARD 20)
endif()
message(STATUS "CMAKE_CXX_STANDARD = ${CMAKE_CXX_STANDARD}")

cmake_policy(SET CMP0144 NEW)

add_subdirectory(src/nps)
