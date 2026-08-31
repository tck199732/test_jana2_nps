project(lib_cnpy)

set(LIBRARY_NAME cnpy)

add_library(${LIBRARY_NAME} SHARED
    cnpy.cpp
)

target_include_directories(
    ${LIBRARY_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

install(
    TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS ${LIBRARY_NAME} DESTINATION bin)