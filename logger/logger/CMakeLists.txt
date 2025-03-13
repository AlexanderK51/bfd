cmake_minimum_required(VERSION 3.22)

project(Logger VERSION 1.0 LANGUAGES CXX)

find_package(spdlog REQUIRED)

SET(LOGGER logger)

add_library(${LOGGER}
    src/logger.cpp
)

target_include_directories(${LOGGER} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(${LOGGER} PRIVATE
    -fpermissive
    -Wall
    -Wextra
    -Werror
    -Wpedantic
)

set_target_properties(${LOGGER} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(${LOGGER} PRIVATE
    spdlog::spdlog_header_only
)

if(USE_CLANG_TIDY)
    find_program(CLANG-TIDY_PATH NAMES clang-tidy-19 clang-tidy REQUIRED)
    message(STATUS "Found clang-tidy: ${CLANG-TIDY_PATH}")
    set_target_properties(${LOGGER}
        PROPERTIES
            CXX_CLANG_TIDY ${CLANG-TIDY_PATH}
    )
endif()

if(USE_CLANG_FORMAT)
    find_program(CLANG-FORMAT_PATH NAMES clang-format-19 clang-format REQUIRED)
    message(STATUS "Found clang-format: ${CLANG-FORMAT_PATH}")

    file(GLOB_RECURSE CLANG-FORMAT_SRC CONFIGURE_DEPENDS
        src/*.h
        src/*.cpp
        src/*.hpp
    )

    add_custom_target(clangformat_logger
        COMMAND
            ${CLANG-FORMAT_PATH} --style=file -i ${CLANG-FORMAT_SRC}
        WORKING_DIRECTORY
            ${CMAKE_SOURCE_DIR}
        COMMENT
            "Formatting ${PROJECT_NAME} with ${CLANG-FORMAT_PATH} ..."
    )

    add_dependencies(${LOGGER} clangformat_logger)
endif()
