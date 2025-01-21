  cmake_minimum_required(VERSION 3.30)
  project(.)

  set(CMAKE_CXX_STANDARD 17)

  add_executable(compression gorilla_compression.cpp
          utils.cpp
          data_handlers.cpp
  )
