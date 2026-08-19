cmake_minimum_required(VERSION 3.16)
project(MiniCOpt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(minic
  src/main.cpp
  src/lexer/lexer.cpp
)
target_include_directories(minic PRIVATE include)

enable_testing()
add_test(NAME lexer_gcd
         COMMAND minic --dump-tokens ${CMAKE_CURRENT_SOURCE_DIR}/tests/programs/gcd.c)
add_test(NAME lexer_matmul
         COMMAND minic --dump-tokens ${CMAKE_CURRENT_SOURCE_DIR}/tests/programs/matmul.c)
