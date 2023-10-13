cmake_minimum_required(VERSION 3.5)

project(morsegpt LANGUAGES C)

add_executable(morse main.c)

target_link_libraries(morse m asound)
