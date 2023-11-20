
cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(MddGlu)

add_subdirectory(src)

set(MddGlu_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")
set(MddGlu_LIBRARIES "$<TARGET_NAME:MddGlu_cu>" "$<TARGET_NAME:MddGlu_glu>")

if (NOT PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MddGlu_INCLUDE_DIRS "${MddGlu_INCLUDE_DIRS}" PARENT_SCOPE)
  set(MddGlu_LIBRARIES ${MddGlu_LIBRARIES} PARENT_SCOPE)
  return()
endif()
