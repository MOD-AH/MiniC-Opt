// Cppify — the Bison-generated parser behind Parser's interface
#include "bison_frontend.h"
#include "parser_bison.hpp"

namespace minic {

BisonFrontEnd::BisonFrontEnd(std::vector<Token> tokens, std::string filename)
    : toks_(std::move(tokens)), file_(std::move(filename)) {}

std::unique_ptr<Program> BisonFrontEnd::parseProgram() {
    bisonp::State st(std::move(toks_), sink_);
    bisonp::Parser parser(st);
    parser.parse();
    st.prog->structs = std::move(st.ctx.structs);
    return std::move(st.prog);
}

}  // namespace minic
