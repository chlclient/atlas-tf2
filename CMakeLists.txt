# This file basically dispatches the main build commands done from CLion down the chain to our modules.
# No real compiler work actually gets done here apart from setting up build settings and build defaults.
cmake_minimum_required(VERSION 3.25)

cmake_policy(SET CMP0091 NEW)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_MSVC_RUNTIME_LIBRARY "" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS "/nologo /DWIN32 /D_WINDOWS" CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS_DEBUG "/Ob0 /Od" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG /INCREMENTAL:NO" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "/DEBUG /INCREMENTAL:NO" CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Ob2 /DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/INCREMENTAL:NO" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "/INCREMENTAL:NO" CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO" CACHE STRING "" FORCE)

project(atlas_se LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# This will more than likely not even be hit due to CMake failing compiler tests because of the above parameters.
# But in case something does get past somehow, this will be our final failsafe to preventing unsupported compilers.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    message(STATUS "clang-cl found!")
else ()
    message(FATAL_ERROR "You are using an unsupported compiler. Please install clang-cl from your preferred source.")
endif ()

set(ATLAS_GIT_COMMIT_HASH "UNK")
set(ATLAS_GIT_COMMIT_AUTHOR "UNK")

find_package(Git)
if (GIT_FOUND)
    message(STATUS "Git found!")
    # Fetch git commit hash.
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE ATLAS_GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Fetch git commit author.
    execute_process(
        COMMAND git show -s --format=%an
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE ATLAS_GIT_COMMIT_AUTHOR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else ()
    message(STATUS "Git was not found! Certain information will not be available...")
endif ()

message(STATUS "Git hash: ${ATLAS_GIT_COMMIT_HASH}")
message(STATUS "Git author: ${ATLAS_GIT_COMMIT_AUTHOR}")

add_library(atlas_options INTERFACE)

target_include_directories(atlas_options INTERFACE "${PROJECT_SOURCE_DIR}/base")
target_compile_features(atlas_options INTERFACE cxx_std_23)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(
        atlas_options
        INTERFACE
        _HAS_EXCEPTIONS=0
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        CUR_COMMIT_HASH="${ATLAS_GIT_COMMIT_HASH}"
        CUR_COMMIT_AUTHOR="${ATLAS_GIT_COMMIT_AUTHOR}"
        ATLAS_DEV
        ATLAS_DISABLE_CLOUD
    )
    message(STATUS "Compiling in debug mode.")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(
        atlas_options
        INTERFACE
        _HAS_EXCEPTIONS=0
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        CUR_COMMIT_HASH="${ATLAS_GIT_COMMIT_HASH}"
        CUR_COMMIT_AUTHOR="${ATLAS_GIT_COMMIT_AUTHOR}"
        ATLAS_DISABLE_CLOUD
    )
    message(STATUS "Compiling in test shipping mode. All realtime protections and cloud integration will be disabled!")
else ()
    target_compile_definitions(
        atlas_options
        INTERFACE
        _HAS_EXCEPTIONS=0
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        CUR_COMMIT_HASH="${ATLAS_GIT_COMMIT_HASH}"
        CUR_COMMIT_AUTHOR="${ATLAS_GIT_COMMIT_AUTHOR}"
    )
    message(STATUS "Compiling in shipping mode. All realtime protections and cloud integration will be enabled!")

    set(ATLAS_TF2_VMPROTECT ON)
endif ()

target_compile_options(
    atlas_options
    INTERFACE
    /nologo
    /GS-
    /GR-
    /Zl
    /fp:precise

    /clang:-msse2
    /clang:-mno-avx
    /clang:-mno-avx2
    /clang:-mno-fma
    /clang:-mno-bmi
    /clang:-mno-bmi2
    /clang:-mtune=generic

    /clang:-ffreestanding
    /clang:-fno-exceptions
    /clang:-fno-rtti
    /clang:-fno-stack-protector
    /clang:-fno-threadsafe-statics
    /clang:-fno-unwind-tables
    /clang:-fno-asynchronous-unwind-tables
    /clang:-fno-ident
    /clang:-fno-unroll-loops
)

target_link_options(
    atlas_options
    INTERFACE
    /NOLOGO
    /NODEFAULTLIB
    /SUBSYSTEM:WINDOWS
    /MANIFEST:NO
    /DYNAMICBASE
    /NXCOMPAT
    /OPT:REF
    /OPT:ICF
)

add_library(atlas_options_x86 INTERFACE)

target_link_libraries(atlas_options_x86 INTERFACE atlas_options)
target_compile_options(atlas_options_x86 INTERFACE -m32)
target_link_options(atlas_options_x86 INTERFACE /MACHINE:X86)

# Register our modules.
# This won't compile absolutely everything into 1 dll.
# Instead: CMake targets will allow us to compile in *specifically* what we want for each dll.
# So for example, if you wanted to compile tf2, you would specify "atlas_tf2" in build targets.
# It would then compile the tf2 directory and the base directory, and leave everything else untouched.
# It would also then output a "atlas-tf2.dll" in the CMake build directory.
# However, it is a necessity for us to add these as subdirectories otherwise they will not be registered
# in the main scope and that's BAD!!!
add_subdirectory(tf2)
add_subdirectory(l4d2)
add_subdirectory(cs2018)

add_subdirectory(base)
