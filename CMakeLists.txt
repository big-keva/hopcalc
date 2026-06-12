cmake_minimum_required(VERSION 3.5)
project(hopcalc)

#
# check configuration
#
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

if(MSVC AND NOT DEFINED STATIC_LIB_PREFIX)
  set(STATIC_LIB_PREFIX "lib")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(MSVC)
  add_compile_options(-MP)
else()
  add_compile_options(-fPIC)

  add_compile_options(-pedantic -mfpmath=sse
    -W -Wall -Wextra -Wcast-align -Wfloat-equal -Wpointer-arith -Wformat-security
    -Wmissing-format-attribute -Wformat=1 -Wwrite-strings -Wcast-align -Wno-long-long
    -Woverloaded-virtual -Wno-suggest-attribute=format)

  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-fno-stack-protector -fno-builtin)
  else()
    add_compile_options(-fstack-protector-all -g)
  endif()
endif()

add_library(hopcalc STATIC src/hopcalc.cpp)

add_subdirectory(tests)

#
# install section
#
install(
  TARGETS hopcalc
  LIBRARY DESTINATION lib COMPONENT Rtm)

install(
  TARGETS hopcalc
  ARCHIVE DESTINATION lib COMPONENT Dev)

install(
  FILES hopcalc.hpp
  DESTINATION include/moonycode COMPONENT Dev)

install(
  FILES cmake/HopCalcConfig.cmake 
  DESTINATION lib/cmake/HopCalc COMPONENT Dev)

