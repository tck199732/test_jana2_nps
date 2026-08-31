project(lib_hdevio_modules)

file(GLOB lib_hdevio_modules_src
    *.hpp
    *.cpp
)

set(LIBRARY_NAME hdevio_modules)

add_library(${LIBRARY_NAME} SHARED ${lib_hdevio_modules_src})
target_include_directories(${LIBRARY_NAME}  PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
install(TARGETS ${LIBRARY_NAME}  DESTINATION ${LIBRARY_OUTPUT_DIRECTORY})
