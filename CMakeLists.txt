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

add_library(dd2_render STATIC src/render/renderer.c src/render/mesh_draw.c src/render/camera.c)
target_include_directories(dd2_render PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(dd2_render PRIVATE softgl dd2_assets)
dd2_strict_target(dd2_render)

add_library(dd2_assets STATIC src/assets/archive.c src/assets/level.c src/assets/textures.c src/assets/lz.c src/assets/mesh.c src/assets/scene.c src/assets/track.c)
target_include_directories(dd2_assets PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
dd2_strict_target(dd2_assets)

add_library(dd2_platform STATIC src/platform/file.c src/platform/window.c)
target_include_directories(dd2_platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
if(EMSCRIPTEN)
    target_compile_options(dd2_platform PRIVATE -sUSE_SDL=2)
    target_link_options(dd2_platform INTERFACE -sUSE_SDL=2)
else()
    find_package(SDL2 REQUIRED CONFIG)
    target_link_libraries(dd2_platform PUBLIC SDL2::SDL2)
endif()
dd2_strict_target(dd2_platform)

add_executable(dd2_app src/game/application.c)
target_link_libraries(dd2_app PRIVATE dd2_assets dd2_render dd2_platform)
dd2_strict_target(dd2_app)
if(EMSCRIPTEN)
    target_link_options(dd2_app PRIVATE -sENVIRONMENT=web,worker -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=67108864 -sFORCE_FILESYSTEM=1 -sINVOKE_RUN=0
        "-sEXPORTED_RUNTIME_METHODS=['FS','callMain']"
        "-sEXPORTED_FUNCTIONS=['_main','_dd2_application_select_level','_dd2_application_show_car','_dd2_application_current_level','_dd2_application_current_view','_dd2_application_reset_camera','_dd2_application_release_input']")
    set_target_properties(dd2_app PROPERTIES SUFFIX ".js")
    configure_file(src/platform/web/index.html "${CMAKE_BINARY_DIR}/index.html" COPYONLY)
    configure_file(src/platform/web/viewer.js "${CMAKE_BINARY_DIR}/viewer.js" COPYONLY)
endif()

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

    add_executable(dd2_archive_test tests/rewrite/archive_test.c)
    target_link_libraries(dd2_archive_test PRIVATE dd2_assets)
    dd2_strict_target(dd2_archive_test)
    if(EMSCRIPTEN)
        # Host filesystem access is confined to the test executable. Game assets
        # will enter the shared decoder through the platform's byte storage.
        target_link_options(dd2_archive_test PRIVATE -sEXIT_RUNTIME=1 -sENVIRONMENT=node
            -sALLOW_MEMORY_GROWTH=1 -sNODERAWFS=1)
        set_target_properties(dd2_archive_test PROPERTIES SUFFIX ".js")
        add_test(NAME rewrite_archive COMMAND "${DD2_NODE}" "$<TARGET_FILE:dd2_archive_test>")
    else()
        add_test(NAME rewrite_archive COMMAND dd2_archive_test)
    endif()
    set_tests_properties(rewrite_archive PROPERTIES TIMEOUT 30)

    add_executable(dd2_level_test tests/rewrite/level_test.c)
    target_link_libraries(dd2_level_test PRIVATE dd2_assets)
    dd2_strict_target(dd2_level_test)
    if(EMSCRIPTEN)
        target_link_options(dd2_level_test PRIVATE -sEXIT_RUNTIME=1 -sENVIRONMENT=node
            -sALLOW_MEMORY_GROWTH=1)
        set_target_properties(dd2_level_test PROPERTIES SUFFIX ".js")
        add_test(NAME rewrite_level COMMAND "${DD2_NODE}" "$<TARGET_FILE:dd2_level_test>")
    else()
        add_test(NAME rewrite_level COMMAND dd2_level_test)
    endif()
    set_tests_properties(rewrite_level PROPERTIES TIMEOUT 30)

    add_executable(dd2_mesh_test tests/rewrite/mesh_test.c)
    target_link_libraries(dd2_mesh_test PRIVATE dd2_assets)
    dd2_strict_target(dd2_mesh_test)
    if(EMSCRIPTEN)
        target_link_options(dd2_mesh_test PRIVATE -sEXIT_RUNTIME=1 -sENVIRONMENT=node -sALLOW_MEMORY_GROWTH=1)
        set_target_properties(dd2_mesh_test PROPERTIES SUFFIX ".js")
        add_test(NAME rewrite_mesh COMMAND "${DD2_NODE}" "$<TARGET_FILE:dd2_mesh_test>")
    else()
        add_test(NAME rewrite_mesh COMMAND dd2_mesh_test)
    endif()
    set_tests_properties(rewrite_mesh PROPERTIES TIMEOUT 30)

    add_executable(dd2_texture_render_test tests/rewrite/texture_render_test.c)
    target_link_libraries(dd2_texture_render_test PRIVATE dd2_assets dd2_render softgl)
    dd2_strict_target(dd2_texture_render_test)
    if(EMSCRIPTEN)
        target_link_options(dd2_texture_render_test PRIVATE -sEXIT_RUNTIME=1 -sENVIRONMENT=node
            -sALLOW_MEMORY_GROWTH=1)
        set_target_properties(dd2_texture_render_test PROPERTIES SUFFIX ".js")
        add_test(NAME rewrite_texture_render COMMAND "${DD2_NODE}" "$<TARGET_FILE:dd2_texture_render_test>")
    else()
        add_test(NAME rewrite_texture_render COMMAND dd2_texture_render_test)
    endif()
    set_tests_properties(rewrite_texture_render PROPERTIES TIMEOUT 30)

    add_executable(dd2_mesh_render_test tests/rewrite/mesh_render_test.c)
    target_link_libraries(dd2_mesh_render_test PRIVATE dd2_assets dd2_render softgl)
    dd2_strict_target(dd2_mesh_render_test)
    if(EMSCRIPTEN)
        target_link_options(dd2_mesh_render_test PRIVATE -sEXIT_RUNTIME=1 -sENVIRONMENT=node
            -sALLOW_MEMORY_GROWTH=1)
        set_target_properties(dd2_mesh_render_test PROPERTIES SUFFIX ".js")
        add_test(NAME rewrite_mesh_render COMMAND "${DD2_NODE}" "$<TARGET_FILE:dd2_mesh_render_test>")
    else()
        add_test(NAME rewrite_mesh_render COMMAND dd2_mesh_render_test)
    endif()
    set_tests_properties(rewrite_mesh_render PROPERTIES TIMEOUT 30)

    add_executable(dd2_level_export tests/rewrite/level_export.c)
    target_link_libraries(dd2_level_export PRIVATE dd2_assets)
    dd2_strict_target(dd2_level_export)
    if(EMSCRIPTEN)
        target_link_options(dd2_level_export PRIVATE -sEXIT_RUNTIME=1 -sENVIRONMENT=node
            -sALLOW_MEMORY_GROWTH=1 -sNODERAWFS=1)
        set_target_properties(dd2_level_export PROPERTIES SUFFIX ".js")
    endif()
    add_executable(dd2_mesh_export tests/rewrite/mesh_export.c)
    target_link_libraries(dd2_mesh_export PRIVATE dd2_assets)
    dd2_strict_target(dd2_mesh_export)
    if(EMSCRIPTEN)
        target_link_options(dd2_mesh_export PRIVATE -sEXIT_RUNTIME=1 -sENVIRONMENT=node
            -sALLOW_MEMORY_GROWTH=1 -sNODERAWFS=1)
        set_target_properties(dd2_mesh_export PROPERTIES SUFFIX ".js")
    endif()
    add_executable(dd2_scene_preview tests/rewrite/scene_preview.c)
    target_link_libraries(dd2_scene_preview PRIVATE dd2_assets dd2_render softgl)
    dd2_strict_target(dd2_scene_preview)
    if(EMSCRIPTEN)
        target_link_options(dd2_scene_preview PRIVATE -sEXIT_RUNTIME=1 -sENVIRONMENT=node
            -sALLOW_MEMORY_GROWTH=1 -sNODERAWFS=1)
        set_target_properties(dd2_scene_preview PROPERTIES SUFFIX ".js")
    endif()
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_target(format-check
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/rewrite/quality.py" format-check
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
add_dependencies(dd2_render format-check)
add_dependencies(dd2_assets format-check)
add_dependencies(dd2_platform format-check)
add_custom_target(format
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/rewrite/quality.py" format
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
if(NOT EMSCRIPTEN)
    add_custom_target(tidy
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/rewrite/quality.py"
            tidy --build-dir "${CMAKE_CURRENT_BINARY_DIR}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
endif()
