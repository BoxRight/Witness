#ifndef WITNESS_DRIVER_HPP
#define WITNESS_DRIVER_HPP

#include <string>
#include <fstream>
#include <memory>
#include <iostream>
#include "ast.hpp"
#include "parser.tab.hpp"
#include "location.hh"
#include "semantic_analyzer.hpp"

// Forward declare the lexer class which will be defined in lexer.hpp
namespace witness {
    class Lexer;
}

namespace witness {

class Driver {
public:
    Driver();
    ~Driver();

    // The lexer instance
    std::unique_ptr<Lexer> lexer;

    // Parse a file and build the AST
    // Returns 0 on success, 1 on failure
    int parse(const std::string& filename);

    // Parse from stream
    int parse(std::istream& is);
    
    // This function will be called by the parser to set the final program
    void set_program(Program* p);

    // Get the parsed program
    Program* get_program() const;

    // Run semantic analysis
    void analyze();
    
    // Print the AST
    void print_ast() const;

    // Solver mode management
    void setSolverMode(const std::string& mode);
    std::string getSolverMode() const;

    // Verbosity control
    void setVerbose(bool verbose);
    void setQuiet(bool quiet);
    bool isVerbose() const;
    bool isQuiet() const;

    // Error handling
    void error(const witness::location& l, const std::string& m);
    void error(const std::string& m);

private:
    // The Program AST node, which is the root of our tree
    std::unique_ptr<Program> program;
    
    // The semantic analyzer instance
    std::unique_ptr<SemanticAnalyzer> analyzer;
    
    // Solver mode: "exhaustive" or "external"
    std::string solverMode;
    
    // Verbosity flags
    bool verbose;
    bool quiet;
    
    // Methods to manage the lexer's input stream
    void scan_begin(std::istream& in);
    void scan_end();
    
    // Disallow copying
    Driver(const Driver&) = delete;
    Driver& operator=(const Driver&) = delete;
};

} // namespace witness

#endif // WITNESS_DRIVER_HPP 