#include "driver.hpp"
#include "lexer.hpp" // Defines the Lexer class
#include <fstream>
#include <iostream>

namespace witness {

Driver::Driver() : lexer(nullptr), program(nullptr), analyzer(std::make_unique<SemanticAnalyzer>()), solverMode("exhaustive") {}

Driver::~Driver() {
    // unique_ptr handles cleanup automatically
}

int Driver::parse(const std::string& filename) {
    std::ifstream in_file(filename);
    if (!in_file.good()) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        return 1;
    }

    return parse(in_file);
}

int Driver::parse(std::istream& is) {
    // Create a new lexer for this parse
    lexer = std::make_unique<Lexer>(&is, *this);

    // Instantiate the parser
    Parser parser(*this);
    
    // Parse the input
    int res = parser.parse();

    return res;
}

void Driver::set_program(Program* p) {
    program.reset(p);
}

Program* Driver::get_program() const {
    return program.get();
}

void Driver::analyze() {
    if (!program) {
        std::cerr << "Error: No program to analyze" << std::endl;
        return;
    }
    
    if (!analyzer) {
        std::cerr << "Error: No semantic analyzer available" << std::endl;
        return;
    }
    
    // Pass solver mode to semantic analyzer
    analyzer->setSolverMode(solverMode);
    
    std::cout << "Running semantic analysis..." << std::endl;
    analyzer->analyze(program.get());
}

void Driver::print_ast() const {
    if (!program) {
        std::cerr << "Error: No program to print" << std::endl;
        return;
    }
    
    std::cout << "AST:" << std::endl;
    program->print(std::cout);
}

void Driver::setSolverMode(const std::string& mode) {
    solverMode = mode;
}

std::string Driver::getSolverMode() const {
    return solverMode;
}

void Driver::error(const witness::location& l, const std::string& m) {
    std::cerr << l << ": " << m << std::endl;
}

void Driver::error(const std::string& m) {
    std::cerr << m << std::endl;
}

} // namespace witness 