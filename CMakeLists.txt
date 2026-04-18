cmake_minimum_required(VERSION 3.16)
project(chip8_emu)

find_package(SDL3 REQUIRED)

add_executable(chip8_emu
    src/frontend.c
    src/backend.c
)

target_link_libraries(chip8_emu SDL3::SDL3)

set_target_properties(chip8_emu PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(WIN32)
    add_custom_command(
        TARGET chip8_emu POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:chip8_emu>
        VERBATIM
    )
endif()