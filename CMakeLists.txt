cmake_minimum_required(VERSION 3.10)


# set the project name
project(ORGCopy VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(ORGCopy
	"src/ORGCopy.cpp"
	"src/ORGCopy.h"
	"src/File.cpp"
	"src/File.h"
)

# put the final executable in the bin directory for ease of access
set_target_properties(ORGCopy PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# make the libraries statically linked
target_link_options(ORGCopy PRIVATE "-static")


# MSVC changes
if(MSVC)
	# remove the warnings when using non-"_s" functions
	target_compile_definitions(ORGCopy PRIVATE _CRT_SECURE_NO_WARNINGS)

	# source files are recongnized as /utf-8
	target_compile_options(ORGCopy PRIVATE "/utf-8")

	# statically-link the CRT (so MSVC dlls are not required to run the program)
	foreach(flag_var CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		if(${flag_var} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		endif()
	endforeach()


endif()

