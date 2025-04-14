cmake_minimum_required(VERSION 3.10)
project(CREPL C)

# Set C standard
set(CMAKE_C_STANDARD 11)

# Platform detection
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# Find SDL2 and SDL2_ttf packages
find_package(SDL2 QUIET)
find_package(SDL2_ttf QUIET)

# If SDL2 was not found, set manual paths based on platform
if(NOT SDL2_FOUND)
    if(WIN32)
        # Windows paths
        set(SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL2" CACHE PATH "Path to SDL2 installation")
        set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include" CACHE PATH "Path to SDL2 headers")
        set(SDL2_LIBRARY "${SDL2_DIR}/lib/x64/SDL2.lib;${SDL2_DIR}/lib/x64/SDL2main.lib" CACHE STRING "SDL2 libraries")
    elseif(LINUX)
        # Linux - Try pkg-config approach
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(SDL2 sdl2)
            if(SDL2_FOUND)
                set(SDL2_INCLUDE_DIR ${SDL2_INCLUDE_DIRS})
                set(SDL2_LIBRARY ${SDL2_LIBRARIES})
            else()
                # Fallback to common Linux paths
                set(SDL2_INCLUDE_DIR "/usr/include/SDL2" CACHE PATH "Path to SDL2 headers")
                set(SDL2_LIBRARY "SDL2" CACHE STRING "SDL2 library name")
            endif()
        endif()
    endif()
endif()

# If SDL2_ttf was not found, set manual paths based on platform
if(NOT SDL2_ttf_FOUND)
    if(WIN32)
        # Windows paths
        set(SDL2_TTF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL2_ttf" CACHE PATH "Path to SDL2_ttf installation")
        set(SDL2_TTF_INCLUDE_DIR "${SDL2_TTF_DIR}/include" CACHE PATH "Path to SDL2_ttf headers")
        set(SDL2_TTF_LIBRARY "${SDL2_TTF_DIR}/lib/x64/SDL2_ttf.lib" CACHE STRING "SDL2_ttf library")
    elseif(LINUX)
        # Linux - Try pkg-config approach
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(SDL2_TTF SDL2_ttf)
            if(SDL2_TTF_FOUND)
                set(SDL2_TTF_INCLUDE_DIR ${SDL2_TTF_INCLUDE_DIRS})
                set(SDL2_TTF_LIBRARY ${SDL2_TTF_LIBRARIES})
            else()
                # Fallback to common Linux paths
                set(SDL2_TTF_INCLUDE_DIR "/usr/include/SDL2" CACHE PATH "Path to SDL2_ttf headers")
                set(SDL2_TTF_LIBRARY "SDL2_ttf" CACHE STRING "SDL2_ttf library name")
            endif()
        endif()
    endif()
endif()

# Include directories
include_directories(include ${SDL2_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIR})

# Source files - explicitly list source files instead of using GLOB
# This ensures CMake properly detects when files are added/removed
set(SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/repl_core.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/repl_ui.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/repl_input.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/repl_variables.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/repl_history.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/repl_eval.c"
)
# Add setjmp_alias.c for Windows to fix _setjmp/_longjmp linking
if (WIN32)
    list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/setjmp_alias.c")
endif()

# Create executable
add_executable(crepl ${SOURCES})

# Link libraries
if(SDL2_FOUND AND SDL2_ttf_FOUND)
    target_link_libraries(crepl SDL2::SDL2main SDL2::SDL2 SDL2_ttf::SDL2_ttf)
else()
    if(WIN32)
        target_link_libraries(crepl ${SDL2_LIBRARY} ${SDL2_TTF_LIBRARY})
        
        # For Windows: Copy the DLLs to binary directory
        add_custom_command(TARGET crepl POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_DIR}/lib/x64/SDL2.dll"
            $<TARGET_FILE_DIR:crepl>
        )
        
        add_custom_command(TARGET crepl POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_TTF_DIR}/lib/x64/SDL2_ttf.dll"
            $<TARGET_FILE_DIR:crepl>
        )
    elseif(LINUX)
        # On Linux, we can use the simpler form since the libraries are in the system paths
        target_link_libraries(crepl SDL2 SDL2_ttf m) # 'm' is the math library, often needed
    endif()
endif()

# Set subsystem properties per platform
if(WIN32)
    set_target_properties(crepl PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(crepl mingw32)
    
    # Add msvcrt for setjmp function needed by SDL2_ttf
    target_link_options(crepl PRIVATE -lmsvcrt)
endif()

# Add compile definitions for platform-specific code
if(WIN32)
    target_compile_definitions(crepl PRIVATE PLATFORM_WINDOWS=1)
elseif(LINUX)
    target_compile_definitions(crepl PRIVATE PLATFORM_LINUX=1)
endif()

# Installation rules
if(UNIX)
    include(GNUInstallDirs)
    install(TARGETS crepl DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()