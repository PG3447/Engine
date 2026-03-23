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
#set(FMOD_DIR ${CMAKE_CURRENT_LIST_DIR}/fmod)
#find_library(FMOD_LIBRARY NAMES fmod fmod_vc PATHS ${FMOD_DIR}/lib)
#add_library(fmod SHARED IMPORTED)
#set_target_properties(fmod PROPERTIES
#		IMPORTED_IMPLIB             ${FMOD_DIR}/lib/fmod_vc.lib
#		IMPORTED_LOCATION           ${FMOD_DIR}/lib/fmod.dll
#		INTERFACE_INCLUDE_DIRECTORIES ${FMOD_DIR}/include
#)
						 
set_target_properties(glad
                      stb_image 
                      assimp 
                      glfw 
                      glm 
                      imgui 
                      spdlog
		              yaml-cpp
		              freetype PROPERTIES FOLDER "thirdparty")

if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "thirdparty")
endif()
