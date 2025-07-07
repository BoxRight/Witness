#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include "ast.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>

namespace witness {

// Forward declaration
class Driver;

// Semantic analyzer for Witness language
class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    
    // Main analysis entry point
    void analyze(Program* program);
    
    // Check if a function name is a join operation
    bool isJoinOperation(const std::string& function_name) const;
    
    // Transform function call to join expression if applicable
    std::unique_ptr<Expression> transformJoinCall(FunctionCallExpression* func_call);
    
    // Validate join operation semantics
    bool validateJoinOperation(const std::string& join_type, 
                              Expression* left_asset, 
                              Expression* right_asset);
    
    // Error reporting
    void reportError(const std::string& message);
    void reportWarning(const std::string& message);
    
private:
    // Set of recognized join operations
    std::unordered_set<std::string> join_operations;
    
    // Error and warning collections
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    // AST traversal methods
    void analyzeStatement(Statement* stmt);
    void analyzeExpression(Expression* expr);
    void analyzeTypeDefinition(TypeDefinition* type_def);
    void analyzeAssetDefinition(AssetDefinition* asset_def);
    void analyzeClauseDefinition(ClauseDefinition* clause_def);
    void analyzeExpressionList(ExpressionList* expr_list);
    
    // Join operation validation helpers
    bool validateUniversalJoin(const std::string& join_type, Expression* left, Expression* right);
    bool validateContextualJoin(const std::string& join_type, Expression* left, Expression* right);
    bool validateReciprocalPattern(Expression* left, Expression* right);
    
    // Type checking helpers
    bool isMovableObjectAsset(Expression* asset);
    bool isNonMovableObjectAsset(Expression* asset);
    bool isPositiveServiceAsset(Expression* asset);
    bool isNegativeServiceAsset(Expression* asset);
    bool isObjectAction(Expression* asset);
    bool isServiceAction(Expression* asset);
};

} // namespace witness

#endif // SEMANTIC_ANALYZER_HPP 