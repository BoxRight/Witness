#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <string>
#include <vector>
#include <memory>

// Forward declarations
class Statement;
class Expression;

// Base class for all AST nodes
class Node {
public:
    virtual ~Node() = default;
    virtual void print(std::ostream& os, int indent = 0) const = 0;
};

// Represents the entire program
class Program : public Node {
public:
    std::vector<std::unique_ptr<Statement>> statements;

    void addStatement(std::unique_ptr<Statement> stmt) {
        statements.push_back(std::move(stmt));
    }

    void print(std::ostream& os, int indent = 0) const override;
};

// Base class for all statements
class Statement : public Node {};

// Base class for all expressions
class Expression : public Node {};

// An identifier
class Identifier : public Expression {
public:
    std::string name;
    Identifier(const std::string& name) : name(name) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << std::string(indent, ' ') << "Identifier(" << name << ")";
    }
};

// A string literal
class StringLiteral : public Expression {
public:
    std::string value;
    StringLiteral(const std::string& value) : value(value) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << std::string(indent, ' ') << "StringLiteral(\"" << value << "\")";
    }
};

// A list of expressions
class ExpressionList : public Node {
public:
    std::vector<std::unique_ptr<Expression>> expressions;

    void addExpression(std::unique_ptr<Expression> expr) {
        expressions.push_back(std::move(expr));
    }

    void print(std::ostream& os, int indent = 0) const override {
        for (const auto& expr : expressions) {
            expr->print(os, indent);
            if (expr.get() != expressions.back().get()) {
                os << ", ";
            }
        }
    }
};

// Represents a type definition: object, service, etc.
class TypeDefinition : public Statement {
public:
    std::string type_keyword; // "object", "service", etc.
    std::unique_ptr<Identifier> name;
    std::unique_ptr<ExpressionList> properties;

    TypeDefinition(const std::string& keyword, std::unique_ptr<Identifier> name, std::unique_ptr<ExpressionList> props)
        : type_keyword(keyword), name(std::move(name)), properties(std::move(props)) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << std::string(indent, ' ') << "TypeDefinition(" << type_keyword << "): ";
        name->print(os);
        os << " = ";
        properties->print(os);
        os << ";";
    }
};

// Represents an asset definition
class AssetDefinition : public Statement {
public:
    std::unique_ptr<Identifier> name;
    std::unique_ptr<ExpressionList> value; // List of expressions (subject, action, object, etc.)

    AssetDefinition(std::unique_ptr<Identifier> name, std::unique_ptr<ExpressionList> val)
        : name(std::move(name)), value(std::move(val)) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << std::string(indent, ' ') << "AssetDefinition: ";
        name->print(os);
        os << " = ";
        value->print(os, indent + 2);
        os << ";";
    }
};

// Represents a clause definition
class ClauseDefinition : public Statement {
public:
    std::unique_ptr<Identifier> name;
    std::unique_ptr<Expression> expression;

    ClauseDefinition(std::unique_ptr<Identifier> name, std::unique_ptr<Expression> expr)
        : name(std::move(name)), expression(std::move(expr)) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << std::string(indent, ' ') << "ClauseDefinition: ";
        name->print(os);
        os << " = ";
        expression->print(os, indent + 2);
        os << ";";
    }
};

// Represents a binary operation
class BinaryOpExpression : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    BinaryOpExpression(std::string op, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : op(op), left(std::move(left)), right(std::move(right)) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << std::string(indent, ' ') << "BinaryOp(" << op << ")" << std::endl;
        left->print(os, indent + 2);
        os << std::endl;
        right->print(os, indent + 2);
    }
};

// Represents a unary operation
class UnaryOpExpression : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> operand;

    UnaryOpExpression(std::string op, std::unique_ptr<Expression> operand)
        : op(op), operand(std::move(operand)) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << std::string(indent, ' ') << "UnaryOp(" << op << ")" << std::endl;
        operand->print(os, indent + 2);
    }
};

// Represents a function call like oblig() or global()
class FunctionCallExpression : public Expression {
public:
    std::unique_ptr<Identifier> function_name;
    std::unique_ptr<ExpressionList> arguments;

    FunctionCallExpression(std::unique_ptr<Identifier> name, std::unique_ptr<ExpressionList> args)
        : function_name(std::move(name)), arguments(std::move(args)) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << std::string(indent, ' ') << "FunctionCall: ";
        function_name->print(os);
        os << "(";
        if (arguments) {
            arguments->print(os);
        }
        os << ")";
    }
};

// Represents a validated join operation with specific semantics
class JoinExpression : public Expression {
public:
    std::string join_type;  // "join", "transfer", "sell", etc.
    std::unique_ptr<Expression> left_asset;
    std::unique_ptr<Expression> right_asset;
    
    JoinExpression(const std::string& type, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : join_type(type), left_asset(std::move(left)), right_asset(std::move(right)) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << std::string(indent, ' ') << "JoinOperation(" << join_type << "):" << std::endl;
        left_asset->print(os, indent + 2);
        os << std::endl << std::string(indent, ' ') << "WITH" << std::endl;
        right_asset->print(os, indent + 2);
    }
};

// Forward declaration for semantic analyzer
class SemanticAnalyzer;

// Implementation of Program::print, needs to be after Statement is defined
inline void Program::print(std::ostream& os, int indent) const {
    for (const auto& stmt : statements) {
        if (stmt) { // Guard against null statements (e.g. from empty semicolons)
            stmt->print(os, indent);
            os << std::endl;
        }
    }
}

#endif // AST_HPP 