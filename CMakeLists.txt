cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

project(modelslice C CXX)

MESSAGE(STATUS "This is Project source dir " ${PROJECT_SOURCE_DIR})
MESSAGE(STATUS "This is PROJECT_BINARY_DIR dir " ${PROJECT_BINARY_DIR})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
MESSAGE(STATUS "This isCMAKE_MODULE_PATH  " ${CMAKE_MODULE_PATH})

option(STATIC_STDLIB "Link std library static or dynamic, such as libgcc, libstdc++, libasan" ON)

MESSAGE(STATUS "HOME dir: $ENV{HOME}")
IF(WIN32)
    MESSAGE(STATUS "This is windows.")
    ADD_DEFINITIONS(-DWIN32)
ELSEIF(WIN64)
    MESSAGE(STATUS "This is windows.")
    ADD_DEFINITIONS(-DWIN64)
ELSEIF(APPLE)
    MESSAGE(STATUS "This is apple")
    # normally __MACH__ has already been defined
    ADD_DEFINITIONS(-D__MACH__ )
ELSEIF(UNIX)
    MESSAGE(STATUS "This is UNIX")
    ADD_DEFINITIONS(-DUNIX -DLINUX)
ELSE()
    MESSAGE(STATUS "This is UNKNOW OS")
ENDIF(WIN32)

# This is for clangd plugin for vscode
IF(DEBUG)
    MESSAGE(STATUS "DEBUG has been set as TRUE ${DEBUG}")
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS}  -O0 -g -DDEBUG ")
    ADD_DEFINITIONS(-DENABLE_DEBUG)
ELSEIF(NOT DEFINED ENV{DEBUG})
    MESSAGE(STATUS "Disable debug")
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS}  -O3 -g -fipa-pta -flto=auto")
ELSE()
    MESSAGE(STATUS "Enable debug")
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS}  -O0 -g -DDEBUG")
    ADD_DEFINITIONS(-DENABLE_DEBUG)
ENDIF(DEBUG)

MESSAGE(STATUS "CMAKE_CXX_COMPILER_ID is " ${CMAKE_CXX_COMPILER_ID})
IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${STATIC_STDLIB})
    ADD_LINK_OPTIONS(-static-libgcc -static-libstdc++)
ENDIF()

# ----------------------------- sys include and lib -------------------------------
IF (EXISTS /usr/local/lib)
    LINK_DIRECTORIES (/usr/local/lib)
ENDIF ()
IF (EXISTS /usr/local/lib64)
    LINK_DIRECTORIES (/usr/local/lib64)
ENDIF ()
    INCLUDE_DIRECTORIES(/usr/local/include ${CMAKE_SOURCE_DIR}/include)

# ---------------------------- include and src ------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/sylar)
file(GLOB SRC "${CMAKE_SOURCE_DIR}/sylar/*.cc")


# ------------------------------------ deps --------------------------------------------
set(3RD ${CMAKE_SOURCE_DIR}/deps/3rd/)
include_directories(${3RD}/yaml-cpp/include ${3RD}/boost_1_84_0/include)
link_directories(${3RD}/yaml-cpp/lib ${3RD}/boost_1_84_0/lib)

# ----------------------------------- other --------------------------------------------
set(CMAKE_CXX_FLAGS ${CMAKE_COMMON_FLAGS})
set(CMAKE_C_FLAGS ${CMAKE_COMMON_FLAGS})
message(STATUS "CMAKE_CXX_FLAGS is" ${CMAKE_CXX_FLAGS})

# 源代码相关宏

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#------------------------------------ exec ----------------------------------------------
add_library(sylar ${SRC})
target_link_libraries(sylar pthread yaml-cpp)
# add_subdirectory(tests)
