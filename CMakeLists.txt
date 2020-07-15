cmake_minimum_required(VERSION 3.2)

project(MSH2FBX)

option(BUILD_SHARED_LIB "Build shared lib" OFF)
option(BUILD_STATIC_LIB "Build static lib" OFF)
option(DEBUG "Debug build" OFF)






#SETTINGS


if (BUILD_SHARED_LIB)
	add_library(msh2fbx SHARED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
elseif (BUILD_STATIC_LIB)
	add_library(msh2fbx STATIC)
else()
	add_executable(msh2fbx)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

target_include_directories(msh2fbx PUBLIC /home/will/projects/swbf2wine/libSWBF2ENCODING/LibSWBF2/)

set_property(TARGET msh2fbx PROPERTY CXX_STANDARD 17)
set_property(TARGET msh2fbx PROPERTY CXX_STANDARD_REQUIRED ON)

if (DEBUG)
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

add_compile_definitions(GLM_FORCE_SWIZZLE)
add_compile_definitions(GLM_FORCE_CXX17)
add_compile_definitions(GLM_FORCE_SILENT_WARNINGS)





#SOURCES

if(APPLE)
	target_include_directories(msh2fbx PUBLIC /usr/local/include)
endif()

file(GLOB MSH_FILES MSH2FBX/*.cpp)
file(GLOB CONVERTER_FILES ConverterLib/*.cpp)
file(GLOB DLL_FILES MSH2FBXDLL/*.cpp)

list(APPEND SOURCE_FILES ${MSH_FILES})
list(APPEND SOURCE_FILES ${CONVERTER_FILES})
list(APPEND SOURCE_FILES ${DLL_FILES})

target_sources(msh2fbx PUBLIC ${SOURCE_FILES})





#LINKING

if(APPLE)
	target_link_directories(msh2fbx PUBLIC /usr/local/lib/)
else()
	target_link_directories(msh2fbx PUBLIC /usr/lib/x86_64-linux-gnu)
endif()


#If releasing an executable, remember to make this static
#and replace the debug w/release
target_link_libraries(msh2fbx PUBLIC fbxsdk)

target_link_directories(msh2fbx PUBLIC /home/will/projects/swbf2wine/libSWBF2ENCODING/LibSWBF2/build)
target_link_libraries(msh2fbx PUBLIC libswbf2)

target_link_libraries(msh2fbx PUBLIC fmt)
target_link_libraries(msh2fbx PUBLIC glm_static)
target_link_libraries(msh2fbx PUBLIC -lpthread)
target_link_libraries(msh2fbx PUBLIC m)




if(APPLE)
	target_link_options(msh2fbx PUBLIC -undefined dynamic_lookup)
else()
	#DL must be linked last...seems like something cmake should've understood
	target_link_libraries(msh2fbx PUBLIC dl)
	target_link_libraries(msh2fbx PUBLIC "-Wl,--no-as-needed")
endif()