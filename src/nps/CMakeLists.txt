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

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        12.1.0
)
FetchContent_MakeAvailable(fmt)


# --------- CREATE TDIS EXECUTABLE ----------------
add_executable(nps_onnx_app
    main.cpp
    io/ReplaySource.cpp
    io/RandomSource.cpp
    io/EvioSroBlockSource.cpp

    clustering/BaseOnnxClusterFactory.cpp
    clustering/WaveformClusterFactory.cpp
    clustering/HitClusterFactory.cpp
    clustering/VtpClusterFactory.cpp
    clustering/EvioParsingFactory.cpp
    clustering/OcInferenceFactory.cpp
    
    geometry/NpsGeometryService.cpp
    calibration/fAdc250Service.cpp
    calibration/VtpService.cpp
    onnx/OnnxRuntimeService.cpp

    evio/sro/SroBlockReader.cpp
    evio/sro/SroFrameSetParser.cpp

)

# ---------- FIND REQUIRED PACKAGES -------------
find_package(JANA REQUIRED)
find_package(ROOT COMPONENTS Core RIO Hist Graf Gpad Geom Tree Postscript Matrix Physics MathCore)

find_path(ONNX_RUNTIME_SESSION_INCLUDE_DIRS onnxruntime_cxx_api.h HINTS /usr/local/include/onnxruntime/)
find_library(ONNX_RUNTIME_LIB onnxruntime HINTS /usr/local/lib)

# Get all variables
GET_CMAKE_PROPERTY(vars VARIABLES)

target_include_directories(nps_onnx_app PUBLIC ${CMAKE_CURRENT_LIST_DIR} "${CMAKE_CURRENT_LIST_DIR}/..")
target_link_libraries(nps_onnx_app
    ${JANA_LIB}
    stdc++fs
    ${ROOT_LIBRARIES}
    ${ONNX_RUNTIME_LIB}
    CLI11::CLI11
    fmt::fmt
    cnpy
)

set_target_properties(nps_onnx_app PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
target_include_directories(
    nps_onnx_app 
    SYSTEM PUBLIC 
    ${JANA_INCLUDE_DIR} 
    ${ROOT_INCLUDE_DIRS}
    ${ONNX_RUNTIME_SESSION_INCLUDE_DIRS}
)

# ----------- install destination -------------------
install(TARGETS nps_onnx_app DESTINATION bin)
