project(lib_hdevio)

file(GLOB lib_hdevio_src
    *.h
    *.cc
)

set(LIBRARY_NAME hdevio)

add_library(${LIBRARY_NAME} SHARED ${lib_hdevio_src})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${LIBRARY_NAME})
install(TARGETS ${LIBRARY_NAME}  DESTINATION ${LIBRARY_OUTPUT_DIRECTORY})
