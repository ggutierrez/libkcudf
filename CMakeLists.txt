cmake_minimum_required(VERSION 2.8)

project(KCUDF)
set(KCUDF_VERSION "0.1")
set(KCUDF_NAME "kcudf")
##########################################################################
## user configurable options
##########################################################################
option(BUILD_TOOLS "Build kcudf tools (requires boost program_options)" NO)

##########################################################################
# Boost
##########################################################################
if (BUILD_TOOLS)
  set(BOOST_COMPONENTS)
  list(APPEND BOOST_COMPONENTS system)
  list(APPEND BOOST_COMPONENTS program_options)

  set(Boost_USE_STATIC_LIBS   ON)
  find_package(Boost 1.47.0 COMPONENTS ${BOOST_COMPONENTS})
  if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
  else()
    message(STATUS "Boost is needed to build the tools provided in this package")
    message(STATUS "uild with option -DBUILD_TOOLS=No to disable this feature.")
  endif()
endif()
##########################################################################
# System information
##########################################################################
message(STATUS "Building for architecture: ${CMAKE_SYSTEM}")
##########################################################################
# Additional compiler flags
##########################################################################
include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++0x" C0X_SUPPORT)
if(C0X_SUPPORT)
  message(STATUS "CXX has c++0x support")
  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x")
else()
  message(FATAL_ERROR "c++0x capable compiler is needed to build this project at this time")
endif()

check_cxx_compiler_flag(-Wall FLAG_WALL)
if(FLAG_WALL)
  add_definitions(-Wall)
endif()

check_cxx_compiler_flag(-Wextra FLAG_WEXTRA)
if(FLAG_WEXTRA)
  add_definitions(-Wextra)
endif()

check_cxx_compiler_flag(-fdiagnostics-show-option FLAG_DIAGNOSTIC)
if(FLAG_DIAGNOSTIC)
  add_definitions(-fdiagnostics-show-option)
endif()

## temporal flags for development
add_definitions(-O0 -g)
##########################################################################
# Cudf parser
##########################################################################
find_library(CUDF_PARSER_LIB cudfparser)
find_file(CUDF_PARSER_HDR cudf.h)
if (NOT CUDF_PARSER_LIB)
  message(FATAL_ERROR "cudf parser needed library")
endif()
if (NOT CUDF_PARSER_HDR)
  message(FATAL_ERROR "cudf parser header needed")
endif()
list(APPEND Cudf_LIBRARIES ${CUDF_PARSER_LIB})
message(STATUS "cudf parser: ${CUDF_PARSER_HDR}, ${CUDF_PARSER_LIB}")

##########################################################################
# Library source code
##########################################################################
include_directories(${KCUDF_SOURCE_DIR})

add_library(kcudf
  kcudf/cudf.cpp
  kcudf/cudf.hh
  kcudf/swriter.cpp
  kcudf/swriter.hh
  kcudf/kcudf.cpp
  kcudf/kcudf.hh
  kcudf/reduce.cpp
  kcudf/reduce.hh
  kcudf/gwriter.cpp
  kcudf/gwriter.hh
)

##########################################################################
# Installation                                                           #
##########################################################################
target_link_libraries(kcudf  ${Cudf_LIBRARIES})
install(TARGETS kcudf
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
install(DIRECTORY kcudf/ DESTINATION include/kcudf
  FILES_MATCHING PATTERN "*.hh")  
##########################################################################
# Tools execuables creation
##########################################################################
if (BUILD_TOOLS)
  # Translator
  add_executable(cudf2kcudf tools/translator.cpp)
  target_link_libraries(cudf2kcudf kcudf ${Boost_LIBRARIES})

  add_executable(kcudf-reduce tools/reducer.cpp)
  target_link_libraries(kcudf-reduce kcudf ${Boost_LIBRARIES})

  # Tools insallation
  install(TARGETS cudf2kcudf kcudf-reduce
    RUNTIME DESTINATION bin)
endif()
