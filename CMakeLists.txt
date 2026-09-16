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
  src/parser/parser.cpp
  src/ast/ast.cpp
  src/ast/ast_printer.cpp
  src/sema/sema.cpp
  src/ir/quad.cpp
  src/ir/irgen.cpp
  src/interp/interp.cpp
  src/cfg/cfg.cpp
  src/cfg/dataflow.cpp
  src/opt/pass_manager.cpp
  src/opt/constant_folding.cpp
  src/opt/constant_propagation.cpp
  src/opt/copy_propagation.cpp
  src/opt/common_subexpr.cpp
  src/opt/dead_code_elim.cpp
  src/opt/unreachable_code.cpp
  src/opt/strength_reduction.cpp
  src/opt/licm.cpp
  src/opt/peephole.cpp
  src/backend/emit.cpp
)
target_include_directories(minic PRIVATE include)

add_executable(ast_smoke
  src/ast/ast.cpp
  src/ast/ast_printer.cpp
  tests/unit/ast_smoke_gcd.cpp
)
target_include_directories(ast_smoke PRIVATE include)

enable_testing()
add_test(NAME lexer_gcd
         COMMAND minic --dump-tokens ${CMAKE_CURRENT_SOURCE_DIR}/tests/programs/gcd.c)
add_test(NAME lexer_matmul
         COMMAND minic --dump-tokens ${CMAKE_CURRENT_SOURCE_DIR}/tests/programs/matmul.c)
