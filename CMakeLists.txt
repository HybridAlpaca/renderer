# Project info

cmake_minimum_required (VERSION 3.8)

set (MODULE_NAME "renderer")
set (MODULE_VERSION "1.0.0")

project (${MODULE_NAME} C CXX)

include (GNUInstallDirs)

if ("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")

	message (FATAL_ERROR "In-tree builds are neither supported nor recommended.  See README.md for building information.")

endif ()

# Find dependencies

find_package (glfw3 REQUIRED)

# Describe module

add_library (${MODULE_NAME} SHARED)

target_sources (${MODULE_NAME}
PRIVATE
	"private/renderer.cpp"
	"private/startup.cpp"
	"private/shutdown.cpp"
	"private/swapchain.cpp"
	"private/frame.cpp"
	"private/app-create.cpp"
	"private/app-destroy.cpp"
	"private/window.cpp"
	"private/util.cpp"
	"private/geometry.cpp"
	"private/material.cpp"
	"private/camera.cpp"
	"private/render_component.cpp"

	"private/volk/volk.c"
)

target_link_libraries (${MODULE_NAME}
PRIVATE
	glfw
)

target_include_directories (${MODULE_NAME}
PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
PRIVATE
	"$<BUILD_INTERFACE:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>"
)

set_target_properties (${MODULE_NAME}
PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS OFF
	PREFIX ""
	SUFFIX ".mod"
	PUBLIC_HEADER "public/renderer.h"
)

# Compile shaders

file (GLOB_RECURSE GLSL_SOURCE_FILES "assets/shaders/*.glsl")

set (GLSL_VALIDATOR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/glslangValidator")

foreach (GLSL ${GLSL_SOURCE_FILES})

	get_filename_component (FILE_NAME ${GLSL} NAME)
	set (SPIRV "${CMAKE_INSTALL_PREFIX}/assets/shaders/${FILE_NAME}.spv")

	add_custom_command (
		OUTPUT ${SPIRV}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_INSTALL_PREFIX}/assets/shaders/"
		COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
		DEPENDS ${GLSL}
	)

  	list (APPEND SPIRV_BINARY_FILES ${SPIRV})

endforeach (GLSL)

add_custom_target (shaders DEPENDS ${SPIRV_BINARY_FILES})

add_dependencies (${MODULE_NAME} shaders)

# Install assets

install (DIRECTORY "assets/geometry" DESTINATION "${CMAKE_INSTALL_PREFIX}/assets/")
install (DIRECTORY "assets/image" DESTINATION "${CMAKE_INSTALL_PREFIX}/assets/")

# Install module

install (
	TARGETS ${MODULE_NAME}
	EXPORT ${MODULE_NAME}-targets

	INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# Export module

install (
	EXPORT ${MODULE_NAME}-targets
	NAMESPACE gengine::

	FILE ${MODULE_NAME}-config.cmake
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${MODULE_NAME}"
)
