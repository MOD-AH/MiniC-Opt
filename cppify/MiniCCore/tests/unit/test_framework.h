// MiniC-Opt — minimal unit-test framework
//
// Deliberately hand-written and header-only, for the same reason the lexer
// is hand-written rather than generated: the project carries no external
// dependencies, and vendoring a full framework would be more code than the
// thing it tests. It provides what the suite actually needs — self-
// registering cases, a handful of assertions, per-case failure reporting
// with file and line, and an exit status CI can gate on.
//
// A test registers itself at static-initialisation time:
//
//     TEST(lexer_scans_an_identifier) {
//         CHECK_EQ(lexOne("abc").type, Tok::IDENT);
//     }
//
// CHECK_* records a failure and keeps going, so one broken case reports
// every one of its problems in a single run. REQUIRE_* abandons the case
// immediately, for when continuing would crash (a null pointer, an empty
// vector about to be indexed).
#pragma once
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace minitest {

struct Case {
    std::string name;
    std::string file;
    int line;
    std::function<void()> fn;
};

// Function-local static: the registry is guaranteed to be constructed
// before the first TEST registers into it, whatever order the translation
// units initialise in.
inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failuresInCurrentCase() {
    static int n = 0;
    return n;
}

inline std::vector<std::string>& failureLog() {
    static std::vector<std::string> log;
    return log;
}

// Thrown by REQUIRE_* to abandon the current case without killing the run.
struct Abort {};

struct Registrar {
    Registrar(const char* name, const char* file, int line, std::function<void()> fn) {
        registry().push_back(Case{name, file, line, std::move(fn)});
    }
};

inline void recordFailure(const char* file, int line, const std::string& what) {
    std::ostringstream os;
    os << "      " << file << ":" << line << ": " << what;
    failureLog().push_back(os.str());
    ++failuresInCurrentCase();
}

// Stringify for the failure message. Falls back to a placeholder for types
// that have no operator<<, so CHECK_EQ stays usable on them.
template <typename T>
auto toStr(const T& v, int) -> decltype(std::declval<std::ostream&>() << v, std::string()) {
    std::ostringstream os;
    os << v;
    return os.str();
}
template <typename T>
std::string toStr(const T&, long) {
    return "<unprintable>";
}

inline int runAll(const std::string& filter) {
    int passed = 0, failed = 0;
    for (const Case& c : registry()) {
        if (!filter.empty() && c.name.find(filter) == std::string::npos) continue;
        failuresInCurrentCase() = 0;
        failureLog().clear();
        try {
            c.fn();
        } catch (const Abort&) {
            // REQUIRE_* already logged the reason.
        } catch (const std::exception& e) {
            recordFailure(c.file.c_str(), c.line,
                          std::string("unexpected exception: ") + e.what());
        } catch (...) {
            recordFailure(c.file.c_str(), c.line, "unexpected non-standard exception");
        }
        if (failuresInCurrentCase() == 0) {
            ++passed;
        } else {
            ++failed;
            std::cout << "  FAIL  " << c.name << "\n";
            for (const std::string& line : failureLog()) std::cout << line << "\n";
        }
    }
    std::cout << "\n  " << passed << " passed, " << failed << " failed, "
              << (passed + failed) << " total\n";
    return failed == 0 ? 0 : 1;
}

}  // namespace minitest

#define MT_CAT_(a, b) a##b
#define MT_CAT(a, b) MT_CAT_(a, b)

#define TEST(name)                                                            \
    static void MT_CAT(mt_fn_, name)();                                       \
    static ::minitest::Registrar MT_CAT(mt_reg_, name)(                       \
        #name, __FILE__, __LINE__, MT_CAT(mt_fn_, name));                     \
    static void MT_CAT(mt_fn_, name)()

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) ::minitest::recordFailure(__FILE__, __LINE__,            \
                                               "CHECK failed: " #cond);       \
    } while (0)

#define CHECK_EQ(a, b)                                                        \
    do {                                                                      \
        auto mt_a_ = (a);                                                     \
        auto mt_b_ = (b);                                                     \
        if (!(mt_a_ == mt_b_))                                                \
            ::minitest::recordFailure(                                        \
                __FILE__, __LINE__,                                           \
                std::string("CHECK_EQ failed: " #a " == " #b "  (")           \
                    + ::minitest::toStr(mt_a_, 0) + " vs "                    \
                    + ::minitest::toStr(mt_b_, 0) + ")");                     \
    } while (0)

#define CHECK_NE(a, b)                                                        \
    do {                                                                      \
        if ((a) == (b))                                                       \
            ::minitest::recordFailure(__FILE__, __LINE__,                     \
                                      "CHECK_NE failed: " #a " != " #b);      \
    } while (0)

#define REQUIRE(cond)                                                         \
    do {                                                                      \
        if (!(cond)) {                                                        \
            ::minitest::recordFailure(__FILE__, __LINE__,                     \
                                      "REQUIRE failed: " #cond);              \
            throw ::minitest::Abort{};                                        \
        }                                                                     \
    } while (0)

#define REQUIRE_EQ(a, b)                                                      \
    do {                                                                      \
        auto mt_a_ = (a);                                                     \
        auto mt_b_ = (b);                                                     \
        if (!(mt_a_ == mt_b_)) {                                              \
            ::minitest::recordFailure(                                        \
                __FILE__, __LINE__,                                           \
                std::string("REQUIRE_EQ failed: " #a " == " #b "  (")         \
                    + ::minitest::toStr(mt_a_, 0) + " vs "                    \
                    + ::minitest::toStr(mt_b_, 0) + ")");                     \
            throw ::minitest::Abort{};                                        \
        }                                                                     \
    } while (0)
