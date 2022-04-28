cmake_minimum_required(VERSION 3.10)

project(Level_Renderer_Vulkan)

# currently using unicode in some libraries on win32 but will change soon
ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

if (WIN32)
	# shaderc_combined.lib in Vulkan requires this for debug & release (runtime shader compiling)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MD")
	
	# set the source code
	set(SOURCE main.cpp renderer.cpp XTime.cpp renderer.h defines.h h2bParser.h XTime.h)
	
	# add vertex shader (.hlsl) files here
	set(VERTEX_SHADER VS.hlsl)	
	
	# add pixel shader (.hlsl) files here
	set(PIXEL_SHADER PS.hlsl)

	# add any new c/c++ source code here
	add_executable (Level_Renderer_Vulkan
		${SOURCE}
		${VERTEX_SHADER}
		${PIXEL_SHADER}
	)	
	target_include_directories(Level_Renderer_Vulkan PUBLIC $ENV{VULKAN_SDK}/Include/)
	target_link_directories(Level_Renderer_Vulkan PUBLIC $ENV{VULKAN_SDK}/Lib/)
	# set the vertex shader properties
	set_source_files_properties(${VERTEX_SHADER} PROPERTIES
		VS_SHADER_TYPE Vertex 
		VS_SHADER_MODEL 5.0
		VS_SHADER_ENTRYPOINT main
		VS_TOOL_OVERRIDE "None"
		# VS_TOOL_OVERRIDE "FxCompile" # This sets it to default "HLSL Compiler"
	)
	#set the pixel shader properties
	set_source_files_properties(${PIXEL_SHADER} PROPERTIES
		VS_SHADER_TYPE Pixel
		VS_SHADER_MODEL 5.0
		VS_SHADER_ENTRYPOINT main
		VS_TOOL_OVERRIDE "None"
	)
	#set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VERTEX_SHADER})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PIXEL_SHADER})
endif(WIN32)

if(UNIX AND NOT APPLE)
	# libshaderc_combined.a is required for runtime shader compiling
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lX11 -lshaderc_combined")
    find_package(X11)
	find_package(Vulkan REQUIRED)
    link_libraries(${X11_LIBRARIES})
    include_directories(${X11_INCLUDE_DIR})
    include_directories(${Vulkan_INCLUDE_DIR}) 
	#link_directories(${Vulkan_LIBRARY}) this is currently not working
	link_libraries(${Vulkan_LIBRARIES})
	# the path is (properly)hardcoded because "${Vulkan_LIBRARY}" currently does not 
	# return a proper path on MacOS (it has the .dynlib appended)
    link_libraries(/usr/lib/x86_64-linux-gnu/libshaderc_combined.a)
    add_executable (Level_Renderer_Vulkan main.cpp renderer.cpp XTime.cpp renderer.h defines.h h2bParser.h XTime.h PS.hlsl VS.hlsl)
endif(UNIX AND NOT APPLE)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fmodules -fcxx-modules")
	set(Architecture ${CMAKE_OSX_ARCHITECTURES})
	find_package(Vulkan REQUIRED)
	include_directories(${Vulkan_INCLUDE_DIR}) 
	#link_directories(${Vulkan_LIBRARY}) this is currently not working
	link_libraries(${Vulkan_LIBRARIES})
	# libshaderc_combined.a is required for runtime shader compiling
	# the path is (properly)hardcoded because "${Vulkan_LIBRARY}" currently does not 
	# return a proper path on MacOS (it has the .dynlib appended)
	link_libraries(/usr/local/lib/libshaderc_combined.a)
	add_executable (Level_Renderer_Vulkan main.mm)
endif(APPLE)
