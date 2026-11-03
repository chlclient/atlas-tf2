add_library(atlas_cs2018 SHARED
    entry.cpp
    global.cpp
    hooks.inl
    menu.cpp
    menu.hpp
    config.cpp
    config.hpp
    sdk.cpp
    entities.cpp
    netvars.cpp

    features/anims.cpp
    features/anti_aim.cpp
    features/autowall.cpp
    features/bone_setup.cpp
    features/chams.cpp
    features/compression.cpp
    features/esp.cpp
    features/fakelag.cpp
    features/hitboxes.cpp
    features/hitmarker.cpp
    features/lagcomp.cpp
    features/misc.cpp
    features/model_render.cpp
    features/networking.cpp
    features/nospread.cpp
    features/datamaps.cpp
    features/player_data.cpp
    features/prediction.cpp
    features/ragebot.cpp
    features/resolver.cpp
    features/shot_log.cpp
    features/skins.cpp
    features/viewmodel.cpp

    $<TARGET_OBJECTS:atlas_se_x86>
)

target_include_directories(atlas_cs2018 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(atlas_cs2018 PRIVATE atlas_options_x86)
target_compile_definitions(atlas_cs2018 PUBLIC ATLAS_CS2018=1)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(atlas_cs2018 PRIVATE /ENTRY:atlas_entry)
    target_sources(atlas_cs2018 PRIVATE dev_tables.cpp features/server_hitboxes.cpp)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_link_options(atlas_cs2018 PRIVATE /ENTRY:atlas_entry)
    target_sources(atlas_cs2018 PRIVATE dev_tables.cpp)
else ()
    target_compile_definitions(atlas_cs2018 PRIVATE ATLAS_SHIPPING)
    target_link_options(atlas_cs2018 PRIVATE /NOENTRY "/MAP:${CMAKE_BINARY_DIR}/atlas-cs2018.map")

    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_custom_command(
        TARGET atlas_cs2018
        POST_BUILD
        BYPRODUCTS "${CMAKE_BINARY_DIR}/atlas-cs2018.atlx"
        COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/atlas_pack.py"
        --input "$<TARGET_FILE:atlas_cs2018>"
        --output "${CMAKE_BINARY_DIR}/atlas-cs2018.atlx"
        COMMENT "Packing atlas_cs2018 ATLX v3 payload"
        VERBATIM
    )
    set_property(
        TARGET atlas_cs2018
        PROPERTY LINK_DEPENDS "${PROJECT_SOURCE_DIR}/tools/atlas_pack.py"
    )
endif ()

set_target_properties(
    atlas_cs2018
    PROPERTIES
    CXX_EXTENSIONS OFF
    MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:Embedded>"
    OUTPUT_NAME atlas-cs2018
    PREFIX ""
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
