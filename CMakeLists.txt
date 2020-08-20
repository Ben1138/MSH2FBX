cmake_minimum_required(VERSION 3.2)

project(MSH2FBX)


#SETTINGS

add_executable(msh2fbx)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set_property(TARGET msh2fbx PROPERTY CXX_STANDARD 17)
set_property(TARGET msh2fbx PROPERTY CXX_STANDARD_REQUIRED ON)


#SOURCES

if(APPLE)
	target_include_directories(msh2fbx PUBLIC /usr/local/include)
endif()

#Lazy, but works for a project this simple
file(GLOB MSH_FILES MSH2FBX/*.cpp ConverterLib/*.cpp MSH2FBXDLL/*.cpp)
target_sources(msh2fbx PUBLIC ${MSH_FILES})

set(LIBSWBF2_INCLUDES_DIR $ENV{LIBSWBF2_INCLUDE})
set(LIBSWBF2_LIBS_DIR $ENV{LIBSWBF2_LIB})

if (NOT ${LIBSWBF2_INCLUDES_DIR} EQUAL "")
	target_include_directories(msh2fbx PUBLIC "${LIBSWBF2_INCLUDES_DIR}")
	target_include_directories(msh2fbx PUBLIC "${LIBSWBF2_INCLUDES_DIR}/DirectX")
else()
	message(STATUS "LIBSWBF2_INCLUDE environment variable not set.  
				   	Assuming libSWBF2 headers are your include path already.")
endif()


#LINKING

#Redundant/harmless additions to catch underconfigured systems...
if (APPLE)
	include_directories(/usr/local/include)
	target_link_options(msh2fbx PUBLIC -undefined dynamic_lookup)
	target_link_directories(msh2fbx PUBLIC /usr/local/lib/)
else()
	target_link_libraries(msh2fbx PUBLIC -ldl)
	target_link_directories(msh2fbx PUBLIC /usr/lib/x86_64-linux-gnu/)
endif()


if (NOT ${LIBSWBF2_LIBS_DIR} EQUAL "")
	target_link_directories(msh2fbx PUBLIC ${LIBSWBF2_LIBS_DIR})
else()
	message(STATUS "LIBSWBF2_LIB environment variable not set.  
				   	Assuming libSWBF2 libraries are in your link path already.")
endif()

target_link_libraries(msh2fbx PUBLIC SWBF2 fmt fbxsdk)