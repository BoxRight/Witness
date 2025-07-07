#ifndef WITNESS_LEXER_HPP
#define WITNESS_LEXER_HPP

#if ! defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "parser.tab.hpp"
#include "location.hh"

namespace witness {

class Driver; // Forward declaration

class Lexer : public yyFlexLexer {
public:
    Lexer(std::istream* in, Driver& driver);
    virtual ~Lexer() {}

    // The function that the parser will call to get the next token
    virtual Parser::symbol_type lex();

private:
    Driver& driver; // Reference to the driver for error reporting
    Parser::location_type location; // Current location in the source

    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;
};

// Global yylex function declaration
Parser::symbol_type yylex(Driver& driver);

} // namespace witness

#endif // WITNESS_LEXER_HPP 