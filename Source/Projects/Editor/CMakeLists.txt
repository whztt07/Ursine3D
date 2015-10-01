cmake_minimum_required(VERSION 3.0)

ursine_default_project(Editor
    FOLDER "Engine"
    TYPE EXE
    DEPENDS Boost SDL2 CEF DirectX
    SOURCE_DIR "Source"
    INCLUDE_DIR "Source"
    SYM_LINKS "Assets"
    PARSE_SOURCE_GROUPS
    RECURSIVE_INCLUDES
    BUILD_META
    META_HEADER "Reflection.h"
    PCH_NAME "Precompiled"
)

# disable exceptions
add_definitions(-D_HAS_EXCEPTIONS=0)

# add WITH_EDITOR definition to editor and engine targets
set_target_properties(Editor PROPERTIES COMPILE_DEFINITIONS WITH_EDITOR=1)
set_target_properties(Ursine3D PROPERTIES COMPILE_DEFINITIONS WITH_EDITOR=1)