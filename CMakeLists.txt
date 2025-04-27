cmake_minimum_required(VERSION 3.10)
project(chat_app C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    message(STATUS "Running in WSL/Linux environment")
    set(PLATFORM_LIBS pthread)
endif()

add_executable(chat_app
    src/main.c
    src/network.c
)

target_link_libraries(chat_app PRIVATE ${PLATFORM_LIBS})