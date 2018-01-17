cmake_minimum_required(VERSION 2.8.12)
project(MagnumShadowsExample)

# CMake policies: don't treat imported targets with :: as files
if(POLICY CMP0028)
    cmake_policy(SET CMP0028 NEW)
endif()
# Enable MACOSX_RPATH by default
if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()
# Quoted variables should not be dereferenced
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()

find_package(Magnum REQUIRED
    MeshTools
    Primitives
    Shaders
    SceneGraph
    GlfwApplication
	Audio)

find_package(MagnumExtras REQUIRED
	Ui)

find_package(Corrade REQUIRED Utility)
set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

corrade_add_resource(Shadows_RESOURCES resources.conf)

add_executable(magnum-shadows
	Types.h
    ShadowsExample.cpp
    ShadowCasterDrawable.h
    ShadowCasterDrawable.cpp
    ShadowLight.h
    ShadowLight.cpp
    ShadowCasterShader.cpp
    ShadowCasterShader.h
    ShadowReceiverDrawable.cpp
    ShadowReceiverDrawable.h
    ShadowReceiverShader.cpp
    ShadowReceiverShader.h
    DebugLines.h
    DebugLines.cpp
	Shadows.cpp
	Shadows.h
    ${Shadows_RESOURCES})
target_link_libraries(magnum-shadows
    Magnum::Application
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::SceneGraph
    Magnum::Shaders
	Magnum::Audio
	Corrade::Utility
	MagnumExtras::Ui)

target_include_directories(magnum-shadows PRIVATE
	"P:/sys/include"
	${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS magnum-shadows DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
install(FILES README.md DESTINATION ${MAGNUM_DATA_INSTALL_DIR}/examples RENAME README-shadows.md)
