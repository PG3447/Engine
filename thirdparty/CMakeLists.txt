# CPM - package manager
include(CPM)

# OpenGL
find_package(OpenGL REQUIRED)

# glad
set(GLAD_DIR ${CMAKE_CURRENT_LIST_DIR}/glad)
set(glad_SOURCE_DIR ${GLAD_DIR}/include CACHE INTERNAL "")
add_library(glad STATIC ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PRIVATE ${GLAD_DIR}/include)

# stb_image
set(STB_IMAGE_DIR ${CMAKE_CURRENT_LIST_DIR}/stb_image)
set(stb_image_SOURCE_DIR ${STB_IMAGE_DIR} CACHE INTERNAL "")
add_library(stb_image STATIC ${STB_IMAGE_DIR}/stb_image.cpp)
target_include_directories(stb_image PRIVATE ${STB_IMAGE_DIR})

# other
CPMAddPackage("gh:assimp/assimp@5.4.3")
CPMAddPackage("gh:glfw/glfw#3.3.8")
CPMAddPackage("gh:g-truc/glm#1.0.1")
CPMAddPackage("gh:ocornut/imgui@1.88")
CPMAddPackage("gh:gabime/spdlog@1.10.0")
CPMAddPackage(
		NAME yaml-cpp
		GITHUB_REPOSITORY jbeder/yaml-cpp
		GIT_TAG 0.8.0
		OPTIONS
		"CMAKE_POLICY_VERSION_MINIMUM 3.5"
		"YAML_CPP_BUILD_TESTS OFF"
		"YAML_CPP_BUILD_TOOLS OFF"
		"YAML_CPP_INSTALL OFF"
)
CPMAddPackage(
		NAME freetype
		GITHUB_REPOSITORY freetype/freetype
		GIT_TAG VER-2-14-3
)

set(imgui_SOURCE_DIR ${imgui_SOURCE_DIR} CACHE INTERNAL "")
add_library(imgui STATIC ${imgui_SOURCE_DIR}/imgui.cpp
					     ${imgui_SOURCE_DIR}/imgui_demo.cpp
					     ${imgui_SOURCE_DIR}/imgui_draw.cpp
						 ${imgui_SOURCE_DIR}/imgui_tables.cpp
					     ${imgui_SOURCE_DIR}/imgui_widgets.cpp)

#FMOD
set(FMOD_DIR ${CMAKE_CURRENT_LIST_DIR}/fmod CACHE INTERNAL "")
set(FMOD_INCLUDE_DIR
    "${FMOD_DIR}/api/core/inc"
    "${FMOD_DIR}/api/studio/inc"
    CACHE INTERNAL ""
)

# --- Detect architecture ---
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_DIR "x64" CACHE INTERNAL "")
else()
    set(ARCH_DIR "x86" CACHE INTERNAL "")
endif()

# (opcjonalnie arm64 override)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(ARCH_DIR "arm64" CACHE INTERNAL "")
endif()

# --- Library path ---
set(FMOD_LIB_DIR "${FMOD_DIR}/api/core/lib/${ARCH_DIR}" CACHE INTERNAL "")


# --- Stwórz importowaną bibliotekę ---
add_library(fmod SHARED IMPORTED GLOBAL)

set_target_properties(fmod PROPERTIES
    # Ścieżka do DLL (runtime)
    IMPORTED_LOCATION_DEBUG "${FMOD_LIB_DIR}/fmodL.dll"
    IMPORTED_LOCATION_RELEASE "${FMOD_LIB_DIR}/fmod.dll"

    # Ścieżka do import library (linker)
    IMPORTED_IMPLIB_DEBUG "${FMOD_LIB_DIR}/fmodL_vc.lib"
    IMPORTED_IMPLIB_RELEASE "${FMOD_LIB_DIR}/fmod_vc.lib"

	IMPORTED_LOCATION_RELWITHDEBINFO "${FMOD_LIB_DIR}/fmod.dll"
    IMPORTED_IMPLIB_RELWITHDEBINFO "${FMOD_LIB_DIR}/fmod_vc.lib"

    IMPORTED_LOCATION_MINSIZEREL "${FMOD_LIB_DIR}/fmod.dll"
    IMPORTED_IMPLIB_MINSIZEREL "${FMOD_LIB_DIR}/fmod_vc.lib"

    INTERFACE_INCLUDE_DIRECTORIES "${FMOD_INCLUDE_DIR}"
)

add_library(fmodstudio SHARED IMPORTED GLOBAL)

set_target_properties(fmodstudio PROPERTIES
    # DLL
    IMPORTED_LOCATION_DEBUG "${FMOD_DIR}/api/studio/lib/${ARCH_DIR}/fmodstudioL.dll"
    IMPORTED_LOCATION_RELEASE "${FMOD_DIR}/api/studio/lib/${ARCH_DIR}/fmodstudio.dll"

    IMPORTED_LOCATION_RELWITHDEBINFO "${FMOD_DIR}/api/studio/lib/${ARCH_DIR}/fmodstudio.dll"
    IMPORTED_LOCATION_MINSIZEREL "${FMOD_DIR}/api/studio/lib/${ARCH_DIR}/fmodstudio.dll"

    # LIB
    IMPORTED_IMPLIB_DEBUG "${FMOD_DIR}/api/studio/lib/${ARCH_DIR}/fmodstudioL_vc.lib"
    IMPORTED_IMPLIB_RELEASE "${FMOD_DIR}/api/studio/lib/${ARCH_DIR}/fmodstudio_vc.lib"

    IMPORTED_IMPLIB_RELWITHDEBINFO "${FMOD_DIR}/api/studio/lib/${ARCH_DIR}/fmodstudio_vc.lib"
    IMPORTED_IMPLIB_MINSIZEREL "${FMOD_DIR}/api/studio/lib/${ARCH_DIR}/fmodstudio_vc.lib"

    INTERFACE_INCLUDE_DIRECTORIES "${FMOD_INCLUDE_DIR}"
)

set_target_properties(glad
                      stb_image 
                      assimp 
                      glfw 
                      glm 
                      imgui 
                      spdlog
					  yaml-cpp
					  freetype
					  fmod
                      fmodstudio PROPERTIES FOLDER "thirdparty")

if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "thirdparty")
endif()
