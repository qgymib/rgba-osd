cmake_minimum_required(VERSION 3.5)
project(rgba-osd
    VERSION 1.0.0
    LANGUAGES C
)

###############################################################################
# Functions
###############################################################################

# Enable all reasonable warnings and make all warnings into errors.
function(setup_target_wall name)
    if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${name} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    endif ()
endfunction()

###############################################################################
# executable
###############################################################################

set(rgba_osd_sources
    "rgba-osd.c"
)

add_library(${PROJECT_NAME} ${rgba_osd_sources})

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -DCMAKE_PROJECT_NAME=${CMAKE_PROJECT_NAME}
        -DCMAKE_PROJECT_VERSION=${CMAKE_PROJECT_VERSION}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

setup_target_wall(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${CMAKE_PROJECT_VERSION}
)

###############################################################################
# Dependency
###############################################################################

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(third_party/cutest)
    include(test/CMakeLists.txt)
endif()

