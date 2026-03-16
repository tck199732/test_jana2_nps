cmake_minimum_required(VERSION 3.24)

project(NPS_ONNX_APP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Default to C++ extensions being off. Clang's modules support have trouble
# with extensions right now and it is not required for any other compiler
set(CMAKE_CXX_EXTENSIONS OFF)


# Set the policy to NEW to use the environment variables in uppercase
cmake_policy(SET CMP0144 NEW)


# --------- FETCH DEPENDENCIES --------------
# Fetch CLI11
include(FetchContent)
FetchContent_Declare(
        CLI11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG        v2.5.0
)
FetchContent_MakeAvailable(CLI11)

# --------- CREATE TDIS EXECUTABLE ----------------
add_executable(nps_onnx_app
        main.cpp
        io/ReplaySource.cpp
        geometry/NpsGeometryService.cpp
        clustering/VtpClusterFactory.cpp
        calibration/fAdc250Service.cpp
        calibration/VtpService.cpp
)

# ---------- FIND REQUIRED PACKAGES -------------
find_package(JANA REQUIRED)
find_package(ROOT COMPONENTS Core RIO Hist Graf Gpad Geom Tree Postscript Matrix Physics MathCore)
# Get all variables
GET_CMAKE_PROPERTY(vars VARIABLES)


target_include_directories(nps_onnx_app PUBLIC ${CMAKE_CURRENT_LIST_DIR} "${CMAKE_CURRENT_LIST_DIR}/..")
target_link_libraries(nps_onnx_app
    ${JANA_LIB}
    stdc++fs
    ${ROOT_LIBRARIES}
    CLI11::CLI11
)

set_target_properties(nps_onnx_app PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
target_include_directories(nps_onnx_app SYSTEM PUBLIC ${JANA_INCLUDE_DIR} ${ROOT_INCLUDE_DIRS})
target_include_directories(nps_onnx_app PUBLIC ${CMAKE_CURRENT_LIST_DIR} "${CMAKE_CURRENT_LIST_DIR}/..")

# ----------- install destination -------------------
install(TARGETS nps_onnx_app DESTINATION bin)
