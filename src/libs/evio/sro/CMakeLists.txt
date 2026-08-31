project(lib_evio_sro)

file(GLOB lib_evio_sro_src
    *.hpp
    *.cpp
)

set(LIBRARY_NAME evio_sro)

add_library(${LIBRARY_NAME} SHARED ${lib_evio_sro_src})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${LIBRARY_NAME})
install(TARGETS ${LIBRARY_NAME}  DESTINATION ${LIBRARY_OUTPUT_DIRECTORY})
