add_library(atlas_l4d2 SHARED
    entry.cpp
    global.cpp
    hooks.inl
    menu.cpp
    sdk.cpp
    entities.cpp
    netvars.cpp
    features/chams.cpp
    features/datamaps.cpp
    features/esp.cpp
    features/glow.cpp
    features/hitscan.cpp
    features/lagcomp.cpp
    features/misc.cpp
    features/networking.cpp
    features/nospread.cpp
    features/prediction.cpp
    features/viewmodel.cpp
    $<TARGET_OBJECTS:atlas_se_x86>
)

target_include_directories(atlas_l4d2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(atlas_l4d2 PRIVATE atlas_options_x86)
target_compile_definitions(atlas_l4d2 PUBLIC ATLAS_L4D2=1)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(atlas_l4d2 PRIVATE /ENTRY:atlas_entry)
    target_sources(atlas_l4d2 PRIVATE dev_tables.cpp)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_link_options(atlas_l4d2 PRIVATE /ENTRY:atlas_entry)
    target_sources(atlas_l4d2 PRIVATE dev_tables.cpp)
else ()
    target_compile_definitions(atlas_l4d2 PRIVATE ATLAS_SHIPPING)
    target_link_options(atlas_l4d2 PRIVATE /NOENTRY "/MAP:${CMAKE_BINARY_DIR}/atlas-l4d2.map")

    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_custom_command(
        TARGET atlas_l4d2
        POST_BUILD
        BYPRODUCTS "${CMAKE_BINARY_DIR}/atlas-l4d2.atlx"
        COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/atlas_pack.py"
        --input "$<TARGET_FILE:atlas_l4d2>"
        --output "${CMAKE_BINARY_DIR}/atlas-l4d2.atlx"
        COMMENT "Packing atlas_l4d2 ATLX v3 payload"
        VERBATIM
    )
    set_property(
        TARGET atlas_l4d2
        PROPERTY LINK_DEPENDS "${PROJECT_SOURCE_DIR}/tools/atlas_pack.py"
    )
endif ()

set_target_properties(
    atlas_l4d2
    PROPERTIES
    CXX_EXTENSIONS OFF
    MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:Embedded>"
    OUTPUT_NAME atlas-l4d2
    PREFIX ""
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
