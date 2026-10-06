cmake_minimum_required(VERSION 3.25)
project(dd2_rewrite VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BINARY_DIR MATCHES "^/tmp/wasm-dd2/")
    message(FATAL_ERROR "Use an external build directory under /tmp/wasm-dd2/.")
endif()

set(DD2_SOFTGL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/softgl" CACHE PATH
    "Pinned SoftGL checkout; override with a local checkout for development")
if(NOT EXISTS "${DD2_SOFTGL_DIR}/libsoftgl/CMakeLists.txt")
    message(FATAL_ERROR "Initialize dependencies: git submodule update --init --recursive")
endif()

add_subdirectory("${DD2_SOFTGL_DIR}/libsoftgl" "${CMAKE_CURRENT_BINARY_DIR}/softgl")
set(DD2_COMPILE_OPTIONS
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter -Wno-unused-function
    -fno-strict-aliasing -ffast-math)
target_compile_options(softgl PRIVATE ${DD2_COMPILE_OPTIONS})
target_compile_definitions(softgl PRIVATE _POSIX_C_SOURCE=200809L)
if(EMSCRIPTEN)
    target_compile_options(softgl PRIVATE -msimd128 -msse -msse2 -msse3 -mssse3 -msse4.1 -pthread)
    target_compile_options(softgl INTERFACE -pthread)
    target_link_options(softgl INTERFACE -msimd128 -pthread -sPTHREAD_POOL_SIZE=8)
endif()

option(DD2_ENABLE_CLANG_TIDY "Treat all rewrite clang-tidy findings as errors" ON)
if(DD2_ENABLE_CLANG_TIDY AND NOT EMSCRIPTEN)
    find_program(DD2_CLANG_TIDY NAMES clang-tidy-19 clang-tidy REQUIRED)
    execute_process(COMMAND "${DD2_CLANG_TIDY}" --version
        OUTPUT_VARIABLE DD2_TIDY_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT DD2_TIDY_VERSION MATCHES "version 19\\.")
        message(FATAL_ERROR "clang-tidy 19 is required for reproducible checks.")
    endif()
endif()

function(dd2_strict_target target)
    target_compile_options(${target} PRIVATE ${DD2_COMPILE_OPTIONS}
        -Werror -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2)
    if(DD2_ENABLE_CLANG_TIDY AND NOT EMSCRIPTEN)
        set_property(TARGET ${target} PROPERTY C_CLANG_TIDY
            "${DD2_CLANG_TIDY};--warnings-as-errors=*")
    endif()
endfunction()

add_library(dd2_render STATIC src/render/renderer.c)
target_include_directories(dd2_render PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(dd2_render PRIVATE softgl)
dd2_strict_target(dd2_render)

include(CTest)
if(BUILD_TESTING)
    add_executable(dd2_render_smoke tests/rewrite/render_smoke.c)
    target_link_libraries(dd2_render_smoke PRIVATE dd2_render softgl)
    dd2_strict_target(dd2_render_smoke)
    if(EMSCRIPTEN)
        find_program(DD2_NODE NAMES node REQUIRED)
        target_link_options(dd2_render_smoke PRIVATE -sEXIT_RUNTIME=1 -sENVIRONMENT=node
            -sALLOW_MEMORY_GROWTH=1)
        set_target_properties(dd2_render_smoke PROPERTIES SUFFIX ".js")
        add_test(NAME rewrite_render_smoke COMMAND "${DD2_NODE}" "$<TARGET_FILE:dd2_render_smoke>")
    else()
        add_test(NAME rewrite_render_smoke COMMAND dd2_render_smoke)
    endif()
    set_tests_properties(rewrite_render_smoke PROPERTIES TIMEOUT 30)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_target(format-check
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/rewrite/quality.py" format-check
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
add_dependencies(dd2_render format-check)
add_custom_target(format
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/rewrite/quality.py" format
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
if(NOT EMSCRIPTEN)
    add_custom_target(tidy
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/rewrite/quality.py"
            tidy --build-dir "${CMAKE_CURRENT_BINARY_DIR}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
endif()
