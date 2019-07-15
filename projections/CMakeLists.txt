# This file is part of ECVL an European Computer Vision Library.
# Developed within the DeepHealth project.
# Boosting AI in Europe.
#
# The MIT License (MIT)
#
# Copyright (c) 2019
#           Federico Bolelli, <federico.bolelli@unimore.it>
#           Costantino Grana, <costantino.grana@unimore.it>
#			Stefano Allegretti, <stefano.allegretti@unimore.it>
#			Laura Canalini, <laura.canalini@unimore.it>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required (VERSION 3.13)

project (projections CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "")

#--------------------------------------------------------------------------------------------------------
# BEGIN: Check for c++17 support
#--------------------------------------------------------------------------------------------------------
# This should be done as follows but it doesn't work!?
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
    
    if(COMPILER_SUPPORTS_CXX17 AND NOT CMAKE_CXX_FLAGS MATCHES "-std=c\\+\\+17")
        set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}" CACHE STRING "Enable c++17" FORCE)
    elseif(NOT COMPILER_SUPPORTS_CXX17)
        message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
    endif()
else()
    if(MSVC)
        if(MSVC_VERSION LESS 1914)
            message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
        endif()
    else()
        message(STATUS "We are not able to check if ${CMAKE_CXX_COMPILER} compiler has C++17 support. ECVL may not work properly.")
    endif()
endif()
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C flags: ${CMAKE_C_FLAGS}")
#--------------------------------------------------------------------------------------------------------
# END: Check for c++17 support
#--------------------------------------------------------------------------------------------------------

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

#--------------------------------------------------------------------------------------------------------
# BEGIN: static/dinamic runtime 												
#--------------------------------------------------------------------------------------------------------
set(projections_STATIC ON CACHE BOOL "Whether to build static or dinamic libraries")

if(MSVC)
    if(projections_STATIC)
        message(STATUS "Setting static runtime for MSVC")
        foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO) 
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endforeach(flag_var)
    else()
	    message(STATUS "Setting dinamic runtime for MSVC")
        foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO) 
            string(REGEX REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
        endforeach(flag_var)
	endif()
endif()
#--------------------------------------------------------------------------------------------------------
# END: static/dinamic runtime										
#--------------------------------------------------------------------------------------------------------

add_executable(projections "projections.cpp")

find_package(ECVL REQUIRED gui)
target_link_libraries(projections ${ECVL_LIBRARIES})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
target_include_directories(projections PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
