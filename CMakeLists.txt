
cmake_minimum_required(VERSION 3.16)

execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)


project(
    backup
    VERSION 0.0.1
    LANGUAGES CXX C
)

add_definitions(
    "-DGIT_HASH=\"${GIT_HASH}\""
    "-DVERSION=\"${CMAKE_PROJECT_VERSION}\""
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)


add_compile_options(-Wall -Wextra -pedantic -Werror)

add_subdirectory(
    src
)

