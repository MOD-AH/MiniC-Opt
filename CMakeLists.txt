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

# The unit suite links every compiler source except main.cpp (it brings its
# own entry point) and self-registers its cases at static-init time, so a
# new tests/unit/test_*.cpp only needs adding to this list.
add_executable(unit_tests
  tests/unit/unit_main.cpp
  tests/unit/test_lexer.cpp
  tests/unit/test_symbol_cfg.cpp
  tests/unit/test_dataflow_ir.cpp
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
target_include_directories(unit_tests PRIVATE include tests/unit)

enable_testing()
add_test(NAME lexer_gcd
         COMMAND minic --dump-tokens ${CMAKE_CURRENT_SOURCE_DIR}/tests/programs/gcd.c)
add_test(NAME lexer_matmul
         COMMAND minic --dump-tokens ${CMAKE_CURRENT_SOURCE_DIR}/tests/programs/matmul.c)
# ast_smoke was built but never registered, so the project's only unit test
# had never actually executed in CI. Registered here and run by the ctest
# step in .github/workflows/ci.yml.
add_test(NAME ast_smoke COMMAND ast_smoke)
add_test(NAME unit_tests COMMAND unit_tests)
