cmake_minimum_required(VERSION 3.10)
project(BattleCityC)

set(CMAKE_C_STANDARD 11)

# Compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0 -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    add_link_options(-g)
else()
    add_compile_options(-O2 -march=native -flto -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    add_link_options(-flto)
endif()

option(DEBUG_SCREENS "Enable debug screen" OFF)
if(DEBUG_SCREENS)
    add_definitions(-DDEBUG_SCREENS)
endif()

# SDL2 / SDL3 selection
option(USE_SDL3 "Use SDL3 instead of SDL2" OFF)

if(USE_SDL3)
    find_package(SDL3 REQUIRED)
    set(SDL_LIBRARIES SDL3::SDL3)
    set(SDL_INCLUDE_DIRS /usr/include/SDL3)
    add_definitions(-DUSE_SDL3)
else()
    find_package(SDL2 REQUIRED)
    set(SDL_LIBRARIES ${SDL2_LIBRARIES})
    set(SDL_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
endif()

# Генерируем Си-файл с встроенным BMP
file(READ ${CMAKE_SOURCE_DIR}/data/chr.bmp chr_hex_content HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " chr_c_array ${chr_hex_content})
file(WRITE ${CMAKE_BINARY_DIR}/chr_bmp.c
    "const unsigned char chr_bmp_data[] = {\n${chr_c_array}\n};\n"
    "const unsigned int chr_bmp_len = sizeof(chr_bmp_data);\n"
)

file(GLOB SOURCES "src/*.c")
file(GLOB NES_SOURCES "src/nes/*.c")
file(GLOB GAME_SOURCES "src/game/*.c")
list(APPEND SOURCES ${NES_SOURCES} ${GAME_SOURCES})

add_executable(battle_city ${SOURCES} ${CMAKE_BINARY_DIR}/chr_bmp.c)
target_link_libraries(battle_city PRIVATE ${SDL_LIBRARIES})

target_include_directories(battle_city PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/game
    ${CMAKE_SOURCE_DIR}/src/nes
    ${SDL_INCLUDE_DIRS}
)
