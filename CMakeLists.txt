cmake_minimum_required(VERSION 3.28.3)
project(LinuxSockets C)

set(CMAKE_C_STANDARD 11)

add_executable(LinuxSockets main.c
        Sockets.h)
