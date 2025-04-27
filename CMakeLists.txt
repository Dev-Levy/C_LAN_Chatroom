cmake_minimum_required(VERSION 3.10)
project(chat LANGUAGES C)

set(CMAKE_C_COMPILER "gcc")
#---------------------------------------------------------------------
# Kernel Module (.ko) - Built using the kernel's build system
#---------------------------------------------------------------------
set(KERNEL_MODULE_NAME "chardev_db")
set(KERNEL_SOURCES
    src/char_device.c
)

# Generate Kbuild file for the kernel module
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Kbuild.in
    ${CMAKE_CURRENT_BINARY_DIR}/Kbuild
    @ONLY
)

# Custom target to build the kernel module
add_custom_target(
    ${KERNEL_MODULE_NAME}.ko ALL
    COMMAND make -C /lib/modules/${CMAKE_SYSTEM_VERSION}/build M=${CMAKE_CURRENT_BINARY_DIR} modules
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building kernel module ${KERNEL_MODULE_NAME}.ko"
)

# Copy the .ko file to build directory
add_custom_command(
    TARGET ${KERNEL_MODULE_NAME}.ko
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${KERNEL_MODULE_NAME}.ko ${CMAKE_CURRENT_BINARY_DIR}/
)

#---------------------------------------------------------------------
# User-space Program - Built with standard GCC
#---------------------------------------------------------------------
set(USER_SOURCES
    src/main.c
    src/network.c
)

# Build user-space executable
add_executable(chat ${USER_SOURCES})

# Link libraries if needed (e.g., pthread)
target_link_libraries(chat pthread)

#---------------------------------------------------------------------
# Optional: Install targets
#---------------------------------------------------------------------
install(TARGETS chat DESTINATION /usr/local/bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_MODULE_NAME}.ko DESTINATION /lib/modules/${CMAKE_SYSTEM_VERSION}/extra)