cmake_minimum_required(VERSION 3.15)

# Project name - Change this to your plugin name
project(gpu VERSION 1.0.0)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# Build shared library (plugin)
add_library(${PROJECT_NAME} SHARED
    plugin.cpp
)

# Platform-specific configuration
# whatsmycli expects: linux.so, windows.dll, macos.dylib
if(LINUX)
    # Linux: linux.so
    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "linux"
        SUFFIX ".so"
    )
elseif(WIN32)
    # Windows: windows.dll
    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "windows"
        SUFFIX ".dll"
    )
elseif(APPLE)
    # macOS: macos.dylib
    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "macos"
        SUFFIX ".dylib"
    )
endif()

# Platform-specific libraries
if(WIN32)
    # Windows requires setupapi for GPU detection
    target_link_libraries(${PROJECT_NAME} PRIVATE setupapi)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang flags
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -O3
    )
elseif(MSVC)
    # MSVC flags
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /O2
    )
endif()

# Installation (optional)
# Uncomment if you want 'make install' to copy the plugin
# install(TARGETS ${PROJECT_NAME}
#     LIBRARY DESTINATION lib/whatsmy/plugins/${PROJECT_NAME}
#     RUNTIME DESTINATION lib/whatsmy/plugins/${PROJECT_NAME}
# )

# Print build information
message(STATUS "Building plugin: ${PROJECT_NAME}")
message(STATUS "Plugin version: ${PROJECT_VERSION}")
if(LINUX)
    message(STATUS "Platform: Linux")
    message(STATUS "Output: linux.so")
elseif(WIN32)
    message(STATUS "Platform: Windows")
    message(STATUS "Output: windows.dll")
elseif(APPLE)
    message(STATUS "Platform: macOS")
    message(STATUS "Output: macos.dylib")
endif()

