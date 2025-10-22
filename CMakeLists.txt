cmake_minimum_required(VERSION 3.15)
project(PlatformerGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)

add_executable(PlatformerGame PlatformerGames.cpp)

# Только SDL2 пока что
target_link_libraries(PlatformerGame PRIVATE SDL2::SDL2main SDL2::SDL2)

# Указываем, что это консольное приложение
set_target_properties(PlatformerGame PROPERTIES
    WIN32_EXECUTABLE OFF
)

# Дополнительные библиотеки для Windows
if(WIN32)
    target_link_libraries(PlatformerGame PRIVATE gdi32)
endif()