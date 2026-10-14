// MiniC-Opt — unit-test runner entry point.
//
// Every test_*.cpp in this directory self-registers; this file only starts
// the run. An optional argument filters by substring:
//     ./unit_tests cfg_
#include "test_framework.h"

int main(int argc, char** argv) {
    std::string filter = (argc > 1) ? argv[1] : "";
    std::cout << "MiniC-Opt unit tests\n";
    return minitest::runAll(filter);
}
