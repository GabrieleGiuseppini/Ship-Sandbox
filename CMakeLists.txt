cmake_minimum_required (VERSION 2.6)

project (ShipSandbox)
 
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

####################################################
#                External libraries 
#
# The library paths I'm using are from my MSVC build
# environment; just replace my paths with yours.
# For platforms with no difference between Debug and
# Release builds, just use the same library for both.
####################################################

list(APPEND CMAKE_PREFIX_PATH "C:/Users/Neurodancer/source/SDK/DevIL")
list(APPEND CMAKE_PREFIX_PATH "C:/Users/Neurodancer/source/SDK/PicoJSON")
list(APPEND CMAKE_PREFIX_PATH "C:/Users/Neurodancer/source/SDK/SFML")
list(APPEND CMAKE_LIBRARY_PATH "C:/Users/Neurodancer/source/SDK/DevIL")
set(SFML_ROOT "C:/Users/Neurodancer/source/SDK/SFML")
set(wxWidgets_ROOT_DIR "C:/Users/Neurodancer/source/SDK/wxWidgets")

find_path(PICOJSON_INCLUDE_DIRS
    NAMES picojson/picojson.h)

find_package(wxWidgets REQUIRED base gl core media)

find_package(DevIL REQUIRED)
if (WIN32)
	find_path(DEVIL_LIB_DIR
		NAMES DevIL.lib
		PATH_SUFFIXES DevIL libx32 lib64 lib lib32)
	file(GLOB DEVIL_RUNTIME_LIBRARIES 
		${DEVIL_LIB_DIR}/*.dll)
endif()

find_package(SFML COMPONENTS system audio REQUIRED)
if (WIN32)
	find_path(SFML_BIN_DIR
		NAMES sfml-system-2.dll openal32.dll
		PATH_SUFFIXES SFML bin)

	file(GLOB SFML_RUNTIME_RELEASE_LIBRARIES 
		${SFML_BIN_DIR}/*.dll)
	list(FILTER SFML_RUNTIME_RELEASE_LIBRARIES EXCLUDE REGEX "-d-") 

	file(GLOB SFML_RUNTIME_DEBUG_LIBRARIES 
		${SFML_BIN_DIR}/*.dll)
	list(FILTER SFML_RUNTIME_DEBUG_LIBRARIES INCLUDE REGEX "-d-") 
	list(APPEND SFML_RUNTIME_DEBUG_LIBRARIES ${SFML_BIN_DIR}/openal32.dll) 

endif()


find_package(OpenGL REQUIRED)


####################################################
# Flags
####################################################

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")

	# Make sure the CRT is statically linked, and that we use W4 instead of W3
	foreach(flag_var
		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		if(${flag_var} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/MD")
		if(${flag_var} MATCHES "/W3")
			string(REGEX REPLACE "/W3" "" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/W3")
		set(${flag_var} "${${flag_var}} /W4")
	endforeach(flag_var)
	
	if(CMAKE_EXE_LINKER_FLAGS_DEBUG MATCHES "/INCREMENTAL")		
		string(REGEX REPLACE "/INCREMENTAL" "" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
	endif(CMAKE_EXE_LINKER_FLAGS_DEBUG MATCHES "/INCREMENTAL")

	string(REGEX REPLACE "/O2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:fast /Ox /GS-")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /fp:fast /Ox /GS-")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fp:strict /DFLOATING_POINT_CHECKS")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
elseif(CMAKE_COMPILER_IS_GNUXX)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W4 -O3 -ffast-math -fno-math-errno -funsafe-math-optimizations -ffinite-math-only -fno-trapping-math")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /W4 -O3 -ffast-math -fno-math-errno -funsafe-math-optimizations -ffinite-math-only -fno-trapping-math")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fp:strict -DFLOATING_POINT_CHECKS")
endif()
 


message ("cxx Flags:" ${CMAKE_CXX_FLAGS})
message ("cxx Flags Release:" ${CMAKE_CXX_FLAGS_RELEASE})
message ("cxx Flags RelWithDebInfo:" ${CMAKE_CXX_FLAGS_RELEASE})
message ("cxx Flags Debug:" ${CMAKE_CXX_FLAGS_DEBUG})
message ("exe Linker Flags Debug:" ${CMAKE_EXE_LINKER_FLAGS_DEBUG})



####################################################
# Misc
####################################################

if (MSVC)
	set(ADDITIONAL_LIBRARIES "comctl32;rpcrt4;advapi32") # winmm.lib wsock32.lib
else(MSVC)
	set(ADDITIONAL_LIBRARIES "")
endif(MSVC)



####################################################
# Sub-projects
####################################################

add_subdirectory(GameLib)
add_subdirectory(Glad)
add_subdirectory(ShipSandbox)
add_subdirectory(UILib)
add_subdirectory(UnitTests)

