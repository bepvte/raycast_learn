cmake_minimum_required (VERSION 3.18.0)

project(raycast_learn
        LANGUAGES CXX)

find_package(SDL2 REQUIRED)

add_executable(
  ${PROJECT_NAME} WIN32
  main.cpp)
target_link_libraries(
  ${PROJECT_NAME}
  SDL2::SDL2main
  $<IF:$<BOOL:${WIN32}>,SDL2::SDL2-static,SDL2::SDL2>
  m)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
if (WIN32)
  set(CMAKE_EXE_LINKER_FLAGS "-static")
endif (WIN32)
if (EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif (EMSCRIPTEN)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-fno-exceptions -fno-rtti")
