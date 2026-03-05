cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(JANA2_NPS_Calib LANGUAGES CXX)

# --- Find required packages ---
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree)

include_directories(${ROOT_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src)
add_definitions(${ROOT_DEFINITIONS})

# --- JANA2 ---
set(JANA_DIR "" CACHE PATH "Path to JANA cmake directory (e.g. $JANA_HOME/lib/cmake/JANA)")
find_package(JANA REQUIRED)

# --- Handle C++ flags ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Suppress "glog" conflicts with LibTorch/ROOT macros
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)

file(GLOB PROJECT_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_library(JANA2_NPS_Calib ${PROJECT_SOURCES})

# --- Link required libraries for the library target ---
target_link_libraries(
    JANA2_NPS_Calib
    PUBLIC
    ${ROOT_LIBRARIES}
)

# --- Include headers for library consumers (e.g., standalone executables) ---
target_include_directories(
    JANA2_NPS_Calib
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${ROOT_INCLUDE_DIRS}
)

add_subdirectory(plugins)
