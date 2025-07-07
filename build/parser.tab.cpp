// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.





#include "parser.tab.hpp"


// Unqualified %code blocks.
#line 14 "src/parser.ypp"

    #include "driver.hpp"
    #include "lexer.hpp"
    
    // Global yylex function that the parser will call
    witness::Parser::symbol_type yylex(witness::Driver& driver) {
        return driver.lexer->lex();
    }

#line 56 "build/parser.tab.cpp"


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YY_USE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 25 "src/parser.ypp"
namespace witness {
#line 149 "build/parser.tab.cpp"

  /// Build a parser object.
  Parser::Parser (witness::Driver& driver_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      driver (driver_yyarg)
  {}

  Parser::~Parser ()
  {}

  Parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/



  // by_state.
  Parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  Parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  Parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  Parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  Parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  Parser::symbol_kind_type
  Parser::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  Parser::stack_symbol_type::stack_symbol_type ()
  {}

  Parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_function_call: // function_call
        value.YY_MOVE_OR_COPY< Expression* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_opt_expression_list: // opt_expression_list
      case symbol_kind::S_expression_list: // expression_list
        value.YY_MOVE_OR_COPY< ExpressionList* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_identifier_expr: // identifier_expr
        value.YY_MOVE_OR_COPY< Identifier* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_statements: // statements
        value.YY_MOVE_OR_COPY< Program* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_statement: // statement
      case symbol_kind::S_type_definition: // type_definition
      case symbol_kind::S_asset_definition: // asset_definition
      case symbol_kind::S_clause_definition: // clause_definition
        value.YY_MOVE_OR_COPY< Statement* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
      case symbol_kind::S_STRING_LITERAL: // STRING_LITERAL
      case symbol_kind::S_type_keyword: // type_keyword
        value.YY_MOVE_OR_COPY< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  Parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_function_call: // function_call
        value.move< Expression* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_opt_expression_list: // opt_expression_list
      case symbol_kind::S_expression_list: // expression_list
        value.move< ExpressionList* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_identifier_expr: // identifier_expr
        value.move< Identifier* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_statements: // statements
        value.move< Program* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_statement: // statement
      case symbol_kind::S_type_definition: // type_definition
      case symbol_kind::S_asset_definition: // asset_definition
      case symbol_kind::S_clause_definition: // clause_definition
        value.move< Statement* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
      case symbol_kind::S_STRING_LITERAL: // STRING_LITERAL
      case symbol_kind::S_type_keyword: // type_keyword
        value.move< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  Parser::stack_symbol_type&
  Parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_function_call: // function_call
        value.copy< Expression* > (that.value);
        break;

      case symbol_kind::S_opt_expression_list: // opt_expression_list
      case symbol_kind::S_expression_list: // expression_list
        value.copy< ExpressionList* > (that.value);
        break;

      case symbol_kind::S_identifier_expr: // identifier_expr
        value.copy< Identifier* > (that.value);
        break;

      case symbol_kind::S_statements: // statements
        value.copy< Program* > (that.value);
        break;

      case symbol_kind::S_statement: // statement
      case symbol_kind::S_type_definition: // type_definition
      case symbol_kind::S_asset_definition: // asset_definition
      case symbol_kind::S_clause_definition: // clause_definition
        value.copy< Statement* > (that.value);
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
      case symbol_kind::S_STRING_LITERAL: // STRING_LITERAL
      case symbol_kind::S_type_keyword: // type_keyword
        value.copy< std::string > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

  Parser::stack_symbol_type&
  Parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_function_call: // function_call
        value.move< Expression* > (that.value);
        break;

      case symbol_kind::S_opt_expression_list: // opt_expression_list
      case symbol_kind::S_expression_list: // expression_list
        value.move< ExpressionList* > (that.value);
        break;

      case symbol_kind::S_identifier_expr: // identifier_expr
        value.move< Identifier* > (that.value);
        break;

      case symbol_kind::S_statements: // statements
        value.move< Program* > (that.value);
        break;

      case symbol_kind::S_statement: // statement
      case symbol_kind::S_type_definition: // type_definition
      case symbol_kind::S_asset_definition: // asset_definition
      case symbol_kind::S_clause_definition: // clause_definition
        value.move< Statement* > (that.value);
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
      case symbol_kind::S_STRING_LITERAL: // STRING_LITERAL
      case symbol_kind::S_type_keyword: // type_keyword
        value.move< std::string > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  Parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  Parser::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YY_USE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " ("
            << yysym.location << ": ";
        YY_USE (yykind);
        yyo << ')';
      }
  }
#endif

  void
  Parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  Parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  Parser::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  Parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  Parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  Parser::debug_level_type
  Parser::debug_level () const
  {
    return yydebug_;
  }

  void
  Parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  Parser::state_type
  Parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  Parser::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  Parser::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  Parser::operator() ()
  {
    return parse ();
  }

  int
  Parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (driver));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_function_call: // function_call
        yylhs.value.emplace< Expression* > ();
        break;

      case symbol_kind::S_opt_expression_list: // opt_expression_list
      case symbol_kind::S_expression_list: // expression_list
        yylhs.value.emplace< ExpressionList* > ();
        break;

      case symbol_kind::S_identifier_expr: // identifier_expr
        yylhs.value.emplace< Identifier* > ();
        break;

      case symbol_kind::S_statements: // statements
        yylhs.value.emplace< Program* > ();
        break;

      case symbol_kind::S_statement: // statement
      case symbol_kind::S_type_definition: // type_definition
      case symbol_kind::S_asset_definition: // asset_definition
      case symbol_kind::S_clause_definition: // clause_definition
        yylhs.value.emplace< Statement* > ();
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
      case symbol_kind::S_STRING_LITERAL: // STRING_LITERAL
      case symbol_kind::S_type_keyword: // type_keyword
        yylhs.value.emplace< std::string > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2: // program: statements
#line 65 "src/parser.ypp"
               { driver.set_program(yystack_[0].value.as < Program* > ()); }
#line 698 "build/parser.tab.cpp"
    break;

  case 3: // statements: %empty
#line 69 "src/parser.ypp"
                { yylhs.value.as < Program* > () = new Program(); }
#line 704 "build/parser.tab.cpp"
    break;

  case 4: // statements: statements statement
#line 70 "src/parser.ypp"
                           { yystack_[1].value.as < Program* > ()->addStatement(std::unique_ptr<Statement>(yystack_[0].value.as < Statement* > ())); yylhs.value.as < Program* > () = yystack_[1].value.as < Program* > (); }
#line 710 "build/parser.tab.cpp"
    break;

  case 5: // statement: type_definition
#line 74 "src/parser.ypp"
                    { yylhs.value.as < Statement* > () = yystack_[0].value.as < Statement* > (); }
#line 716 "build/parser.tab.cpp"
    break;

  case 6: // statement: asset_definition
#line 75 "src/parser.ypp"
                       { yylhs.value.as < Statement* > () = yystack_[0].value.as < Statement* > (); }
#line 722 "build/parser.tab.cpp"
    break;

  case 7: // statement: clause_definition
#line 76 "src/parser.ypp"
                        { yylhs.value.as < Statement* > () = yystack_[0].value.as < Statement* > (); }
#line 728 "build/parser.tab.cpp"
    break;

  case 8: // statement: SEMICOLON
#line 77 "src/parser.ypp"
                { yylhs.value.as < Statement* > () = nullptr; /* Ignore empty statements */ }
#line 734 "build/parser.tab.cpp"
    break;

  case 9: // type_keyword: OBJECT
#line 81 "src/parser.ypp"
              { yylhs.value.as < std::string > () = "object"; }
#line 740 "build/parser.tab.cpp"
    break;

  case 10: // type_keyword: SERVICE
#line 82 "src/parser.ypp"
                { yylhs.value.as < std::string > () = "service"; }
#line 746 "build/parser.tab.cpp"
    break;

  case 11: // type_keyword: ACTION
#line 83 "src/parser.ypp"
                { yylhs.value.as < std::string > () = "action"; }
#line 752 "build/parser.tab.cpp"
    break;

  case 12: // type_keyword: SUBJECT
#line 84 "src/parser.ypp"
                { yylhs.value.as < std::string > () = "subject"; }
#line 758 "build/parser.tab.cpp"
    break;

  case 13: // type_keyword: AUTHORITY
#line 85 "src/parser.ypp"
                { yylhs.value.as < std::string > () = "authority"; }
#line 764 "build/parser.tab.cpp"
    break;

  case 14: // type_keyword: TIME
#line 86 "src/parser.ypp"
                { yylhs.value.as < std::string > () = "time"; }
#line 770 "build/parser.tab.cpp"
    break;

  case 15: // type_definition: type_keyword identifier_expr EQUALS expression_list SEMICOLON
#line 90 "src/parser.ypp"
                                                                  { yylhs.value.as < Statement* > () = new TypeDefinition(yystack_[4].value.as < std::string > (), std::unique_ptr<Identifier>(yystack_[3].value.as < Identifier* > ()), std::unique_ptr<ExpressionList>(yystack_[1].value.as < ExpressionList* > ())); }
#line 776 "build/parser.tab.cpp"
    break;

  case 16: // asset_definition: ASSET identifier_expr EQUALS expression_list SEMICOLON
#line 94 "src/parser.ypp"
                                                           { yylhs.value.as < Statement* > () = new AssetDefinition(std::unique_ptr<Identifier>(yystack_[3].value.as < Identifier* > ()), std::unique_ptr<ExpressionList>(yystack_[1].value.as < ExpressionList* > ())); }
#line 782 "build/parser.tab.cpp"
    break;

  case 17: // clause_definition: CLAUSE identifier_expr EQUALS expression SEMICOLON
#line 98 "src/parser.ypp"
                                                       { yylhs.value.as < Statement* > () = new ClauseDefinition(std::unique_ptr<Identifier>(yystack_[3].value.as < Identifier* > ()), std::unique_ptr<Expression>(yystack_[1].value.as < Expression* > ())); }
#line 788 "build/parser.tab.cpp"
    break;

  case 18: // expression: identifier_expr
#line 102 "src/parser.ypp"
                                    { yylhs.value.as < Expression* > () = yystack_[0].value.as < Identifier* > (); }
#line 794 "build/parser.tab.cpp"
    break;

  case 19: // expression: STRING_LITERAL
#line 103 "src/parser.ypp"
                                    { yylhs.value.as < Expression* > () = new StringLiteral(yystack_[0].value.as < std::string > ()); }
#line 800 "build/parser.tab.cpp"
    break;

  case 20: // expression: function_call
#line 104 "src/parser.ypp"
                                    { yylhs.value.as < Expression* > () = yystack_[0].value.as < Expression* > (); }
#line 806 "build/parser.tab.cpp"
    break;

  case 21: // expression: expression AND expression
#line 105 "src/parser.ypp"
                                        { yylhs.value.as < Expression* > () = new BinaryOpExpression("AND", std::unique_ptr<Expression>(yystack_[2].value.as < Expression* > ()), std::unique_ptr<Expression>(yystack_[0].value.as < Expression* > ())); }
#line 812 "build/parser.tab.cpp"
    break;

  case 22: // expression: expression OR expression
#line 106 "src/parser.ypp"
                                        { yylhs.value.as < Expression* > () = new BinaryOpExpression("OR", std::unique_ptr<Expression>(yystack_[2].value.as < Expression* > ()), std::unique_ptr<Expression>(yystack_[0].value.as < Expression* > ())); }
#line 818 "build/parser.tab.cpp"
    break;

  case 23: // expression: expression XOR expression
#line 107 "src/parser.ypp"
                                        { yylhs.value.as < Expression* > () = new BinaryOpExpression("XOR", std::unique_ptr<Expression>(yystack_[2].value.as < Expression* > ()), std::unique_ptr<Expression>(yystack_[0].value.as < Expression* > ())); }
#line 824 "build/parser.tab.cpp"
    break;

  case 24: // expression: expression IMPLIES expression
#line 108 "src/parser.ypp"
                                        { yylhs.value.as < Expression* > () = new BinaryOpExpression("IMPLIES", std::unique_ptr<Expression>(yystack_[2].value.as < Expression* > ()), std::unique_ptr<Expression>(yystack_[0].value.as < Expression* > ())); }
#line 830 "build/parser.tab.cpp"
    break;

  case 25: // expression: expression EQUIV expression
#line 109 "src/parser.ypp"
                                        { yylhs.value.as < Expression* > () = new BinaryOpExpression("EQUIV", std::unique_ptr<Expression>(yystack_[2].value.as < Expression* > ()), std::unique_ptr<Expression>(yystack_[0].value.as < Expression* > ())); }
#line 836 "build/parser.tab.cpp"
    break;

  case 26: // expression: NOT expression
#line 110 "src/parser.ypp"
                                        { yylhs.value.as < Expression* > () = new UnaryOpExpression("NOT", std::unique_ptr<Expression>(yystack_[0].value.as < Expression* > ())); }
#line 842 "build/parser.tab.cpp"
    break;

  case 27: // expression: LPAREN expression RPAREN
#line 111 "src/parser.ypp"
                                        { yylhs.value.as < Expression* > () = yystack_[1].value.as < Expression* > (); }
#line 848 "build/parser.tab.cpp"
    break;

  case 28: // function_call: identifier_expr LPAREN opt_expression_list RPAREN
#line 115 "src/parser.ypp"
                                                      { yylhs.value.as < Expression* > () = new FunctionCallExpression(std::unique_ptr<Identifier>(yystack_[3].value.as < Identifier* > ()), std::unique_ptr<ExpressionList>(yystack_[1].value.as < ExpressionList* > ())); }
#line 854 "build/parser.tab.cpp"
    break;

  case 29: // opt_expression_list: %empty
#line 119 "src/parser.ypp"
                      { yylhs.value.as < ExpressionList* > () = new ExpressionList(); }
#line 860 "build/parser.tab.cpp"
    break;

  case 30: // opt_expression_list: expression_list
#line 120 "src/parser.ypp"
                      { yylhs.value.as < ExpressionList* > () = yystack_[0].value.as < ExpressionList* > (); }
#line 866 "build/parser.tab.cpp"
    break;

  case 31: // identifier_expr: IDENTIFIER
#line 124 "src/parser.ypp"
               { yylhs.value.as < Identifier* > () = new Identifier(yystack_[0].value.as < std::string > ()); }
#line 872 "build/parser.tab.cpp"
    break;

  case 32: // expression_list: expression
#line 128 "src/parser.ypp"
               { yylhs.value.as < ExpressionList* > () = new ExpressionList(); yylhs.value.as < ExpressionList* > ()->addExpression(std::unique_ptr<Expression>(yystack_[0].value.as < Expression* > ())); }
#line 878 "build/parser.tab.cpp"
    break;

  case 33: // expression_list: expression_list COMMA expression
#line 129 "src/parser.ypp"
                                       { yystack_[2].value.as < ExpressionList* > ()->addExpression(std::unique_ptr<Expression>(yystack_[0].value.as < Expression* > ())); yylhs.value.as < ExpressionList* > () = yystack_[2].value.as < ExpressionList* > (); }
#line 884 "build/parser.tab.cpp"
    break;


#line 888 "build/parser.tab.cpp"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        std::string msg = YY_("syntax error");
        error (yyla.location, YY_MOVE (msg));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yyerror_range[1].location = yystack_[0].location;
        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  Parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

#if YYDEBUG || 0
  const char *
  Parser::symbol_name (symbol_kind_type yysymbol)
  {
    return yytname_[yysymbol];
  }
#endif // #if YYDEBUG || 0









  const signed char Parser::yypact_ninf_ = -24;

  const signed char Parser::yytable_ninf_ = -1;

  const signed char
  Parser::yypact_[] =
  {
     -24,    11,    23,   -24,   -24,   -24,   -24,   -24,   -24,   -24,
     -24,    15,    15,   -24,    15,   -24,   -24,   -24,   -24,    22,
      32,    38,    -2,    -2,    -2,   -24,    -2,    -2,   -10,   -24,
       9,    46,     7,    48,    24,   -24,    -2,    -2,    -2,    -2,
      -2,    -2,   -24,    -2,   -24,   -24,   -24,   -24,   -24,   -24,
     -10,   -24,    27,    49,   -10,   -24
  };

  const signed char
  Parser::yydefact_[] =
  {
       3,     0,     2,     1,     8,     9,    10,    11,    12,    13,
      14,     0,     0,     4,     0,     5,     6,     7,    31,     0,
       0,     0,     0,     0,     0,    19,     0,     0,    32,    20,
      18,     0,     0,     0,     0,    26,     0,     0,     0,     0,
       0,    29,    16,     0,    17,    15,    27,    21,    22,    23,
      24,    25,     0,    30,    33,    28
  };

  const signed char
  Parser::yypgoto_[] =
  {
     -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -23,   -24,
     -24,    36,   -19
  };

  const signed char
  Parser::yydefgoto_[] =
  {
       0,     1,     2,    13,    14,    15,    16,    17,    28,    29,
      52,    30,    31
  };

  const signed char
  Parser::yytable_[] =
  {
      32,    18,    25,    34,    35,    33,    36,    37,    38,    39,
      40,     3,    44,    47,    48,    49,    50,    51,    18,    26,
      54,    27,    53,    36,    37,    38,    39,    40,     4,    22,
      41,     5,     6,     7,     8,     9,    10,    11,    12,    23,
      36,    37,    38,    39,    40,    24,    46,    19,    20,    55,
      21,    42,    43,    45,    43,    43
  };

  const signed char
  Parser::yycheck_[] =
  {
      23,     3,     4,    26,    27,    24,    16,    17,    18,    19,
      20,     0,     5,    36,    37,    38,    39,    40,     3,    21,
      43,    23,    41,    16,    17,    18,    19,    20,     5,     7,
      21,     8,     9,    10,    11,    12,    13,    14,    15,     7,
      16,    17,    18,    19,    20,     7,    22,    11,    12,    22,
      14,     5,     6,     5,     6,     6
  };

  const signed char
  Parser::yystos_[] =
  {
       0,    25,    26,     0,     5,     8,     9,    10,    11,    12,
      13,    14,    15,    27,    28,    29,    30,    31,     3,    35,
      35,    35,     7,     7,     7,     4,    21,    23,    32,    33,
      35,    36,    32,    36,    32,    32,    16,    17,    18,    19,
      20,    21,     5,     6,     5,     5,    22,    32,    32,    32,
      32,    32,    34,    36,    32,    22
  };

  const signed char
  Parser::yyr1_[] =
  {
       0,    24,    25,    26,    26,    27,    27,    27,    27,    28,
      28,    28,    28,    28,    28,    29,    30,    31,    32,    32,
      32,    32,    32,    32,    32,    32,    32,    32,    33,    34,
      34,    35,    36,    36
  };

  const signed char
  Parser::yyr2_[] =
  {
       0,     2,     1,     0,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     5,     5,     5,     1,     1,
       1,     3,     3,     3,     3,     3,     2,     3,     4,     0,
       1,     1,     1,     3
  };


#if YYDEBUG
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const Parser::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "IDENTIFIER",
  "STRING_LITERAL", "SEMICOLON", "COMMA", "EQUALS", "OBJECT", "SERVICE",
  "ACTION", "SUBJECT", "AUTHORITY", "TIME", "ASSET", "CLAUSE", "AND", "OR",
  "XOR", "IMPLIES", "EQUIV", "LPAREN", "RPAREN", "NOT", "$accept",
  "program", "statements", "statement", "type_keyword", "type_definition",
  "asset_definition", "clause_definition", "expression", "function_call",
  "opt_expression_list", "identifier_expr", "expression_list", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const unsigned char
  Parser::yyrline_[] =
  {
       0,    65,    65,    69,    70,    74,    75,    76,    77,    81,
      82,    83,    84,    85,    86,    90,    94,    98,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   115,   119,
     120,   124,   128,   129
  };

  void
  Parser::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
  Parser::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG


#line 25 "src/parser.ypp"
} // witness
#line 1230 "build/parser.tab.cpp"

#line 132 "src/parser.ypp"


// Error reporting function
void witness::Parser::error(const witness::location& l, const std::string& m) {
    driver.error(l, m);
} 
