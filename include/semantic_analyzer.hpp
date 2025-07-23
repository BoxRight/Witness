#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include "ast.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <set>
#include <map>

namespace witness {

// Forward declaration
class Driver;

// Type information for semantic analysis
struct TypeInfo {
    std::string type_keyword;  // "object", "service", "action", etc.
    std::string constraint;    // "movable", "non_movable", "positive", "negative"
    
    // For assets: store the components (subject, action, object)
    std::vector<std::string> asset_components;
    
    TypeInfo() : type_keyword(""), constraint("") {}
    TypeInfo(const std::string& keyword, const std::string& constraint)
        : type_keyword(keyword), constraint(constraint) {}
    TypeInfo(const std::string& keyword, const std::string& constraint, const std::vector<std::string>& components)
        : type_keyword(keyword), constraint(constraint), asset_components(components) {}
};

// Semantic analyzer for Witness language
class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    
    // Data structures for satisfiability checking
    struct ClauseInfo {
        std::string name;                        // Clause name
        std::vector<int> positive_literals;      // Asset IDs that must be true
        std::vector<int> negative_literals;      // Asset IDs that must be false
        std::string expression;                  // Original expression string
        Expression* expr = nullptr;              // Pointer to the actual clause expression
    };
    
    struct SatisfiabilityResult {
        bool satisfiable;
        std::vector<std::vector<int>> assignments; // All satisfying assignments
        std::string error_message;               // If unsatisfiable
        std::vector<std::string> conflicting_clauses; // Minimal conflict set
    };
    
    // Main analysis entry point
    void analyze(Program* program);
    
    // Solver mode management
    void setSolverMode(const std::string& mode);
    std::string getSolverMode() const;
    
    // Check if a function name is a join operation
    bool isJoinOperation(const std::string& function_name) const;
    
    // Check if a function name is a logical operation
    bool isLogicalOperation(const std::string& function_name) const;
    
    // Check if a function name is a system operation
    bool isSystemOperation(const std::string& function_name) const;
    
    // Transform function call to join expression if applicable
    std::unique_ptr<Expression> transformJoinCall(FunctionCallExpression* func_call);
    
    // Validate logical operation semantics
    bool validateLogicalOperation(const std::string& operation_type, FunctionCallExpression* func_call);
    
    // Validate system operation semantics
    bool validateSystemOperation(const std::string& operation_type, FunctionCallExpression* func_call);
    
    // Check for idempotency: join(x, x) = x
    bool checkIdempotency(const std::string& join_type, Expression* left, Expression* right);
    
    // Lazy asset ID assignment - assign IDs only when assets are used in clauses
    int getOrAssignAssetID(const std::string& asset_name);
    
    // Truth table generation for satisfiability checking
    SatisfiabilityResult generateTruthTable();
    SatisfiabilityResult generateExhaustiveTruthTable();
    void exportForCudaSolver(const std::vector<std::set<std::vector<int>>>& clause_satisfying_assignments, 
                             const std::set<int>& all_asset_ids);
    void generateExternalSolverTruthTable();
    
    // Helper methods for external solver mode
    bool assignmentsCompatible(const std::vector<int>& assignment1, const std::vector<int>& assignment2);
    std::vector<int> mergeAssignments(const std::vector<int>& assignment1, const std::vector<int>& assignment2);
    
    // Clause collection for satisfiability checking
    void addClause(const std::string& clause_name, const std::vector<int>& positive_literals, 
                   const std::vector<int>& negative_literals, const std::string& expression, Expression* expr);
    
    // Validate join operation semantics
    bool validateJoinOperation(const std::string& join_type, 
                              Expression* left_asset, 
                              Expression* right_asset);
    bool validateJoinAssociativity(const std::string& join_type, Expression* left_asset, Expression* right_asset);
    bool validateContextualJoinAssociativity(const std::string& join_type,
                                           const std::vector<std::string>& a_components,
                                           const std::vector<std::string>& b_components,
                                           const std::vector<std::string>& c_components);
    // Helper for per-clause truth table generation
    void collectAssetIDs(Expression* expr, std::set<int>& ids);
    bool evalExpr(Expression* expr, const std::map<int, bool>& assignment);
    
    // Error reporting
    void reportError(const std::string& message);
    void reportWarning(const std::string& message);
    
    // Generate and print the per-clause truth table
    void printClauseTruthTable(const ClauseInfo& clause);
    
private:
    // Set of recognized join operations
    std::unordered_set<std::string> join_operations;
    
    // Set of recognized logical operations
    std::unordered_set<std::string> logical_operations;
    
    // Set of recognized system operations
    std::unordered_set<std::string> system_operations;
    
    // Symbol table for type definitions
    std::unordered_map<std::string, TypeInfo> symbol_table;
    
    // Error and warning collections
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    // Solver mode: "exhaustive" or "external"
    std::string solverMode;
    
    // Asset ID tracking for satisfiability checking
    std::unordered_map<std::string, int> asset_to_id;
    int next_asset_id;
    
    // Current clauses for satisfiability checking
    std::vector<ClauseInfo> current_clauses;
    
    // Symbol table management
    void registerTypeDefinition(TypeDefinition* type_def);
    void registerAssetDefinition(AssetDefinition* asset_def);
    TypeInfo* lookupType(const std::string& identifier);
    
    // AST traversal methods
    void analyzeStatement(Statement* stmt);
    void analyzeExpression(Expression* expr);
    void analyzeClauseExpression(Expression* expr, const std::string& clause_name);
    void analyzeTypeDefinition(TypeDefinition* type_def);
    void analyzeAssetDefinition(AssetDefinition* asset_def);
    void analyzeClauseDefinition(ClauseDefinition* clause_def);
    void analyzeExpressionList(ExpressionList* expr_list);
    
    // Join operation validation helpers
    bool validateUniversalJoin(const std::string& join_type, Expression* left, Expression* right);
    bool validateContextualJoin(const std::string& join_type, Expression* left, Expression* right);
    bool validateReciprocalPattern(Expression* left, Expression* right);
    
    // Logical operation validation helpers
    bool validateObligOperation(FunctionCallExpression* func_call);
    bool validateClaimOperation(FunctionCallExpression* func_call);
    bool validateNotOperation(FunctionCallExpression* func_call);
    
    // System operation validation helpers
    bool validateGlobalOperation(FunctionCallExpression* func_call);
    bool validateLitisOperation(FunctionCallExpression* func_call);
    bool validateMeetOperation(FunctionCallExpression* func_call);
    bool validateDomainOperation(FunctionCallExpression* func_call);
    
    // Specific contextual join validators
    bool validateTransferJoin(Expression* left, Expression* right);
    bool validateSellJoin(Expression* left, Expression* right);
    bool validateCompensationJoin(Expression* left, Expression* right);
    bool validateConsiderationJoin(Expression* left, Expression* right);
    bool validateForbearanceJoin(Expression* left, Expression* right);
    bool validateEncumberJoin(Expression* left, Expression* right);
    bool validateAccessJoin(Expression* left, Expression* right);
    bool validateLienJoin(Expression* left, Expression* right);
    
    // Asset analysis helpers
    bool isAssetExpression(Expression* expr);
    std::vector<std::string> getAssetComponents(Expression* asset);
    std::string getAssetSubject(Expression* asset);
    std::string getAssetAction(Expression* asset);
    std::string getAssetObject(Expression* asset);
    
    // Asset type analysis helpers
    bool analyzeAssetForMovableObject(const std::string& asset_name);
    bool analyzeAssetForNonMovableObject(const std::string& asset_name);
    bool analyzeAssetForPositiveService(const std::string& asset_name);
    bool analyzeAssetForNegativeService(const std::string& asset_name);
    
    // Type system-based asset analysis
    bool analyzeAssetTypeConstraint(const std::string& asset_name, const std::string& expected_type, const std::string& expected_constraint);
    bool analyzeAssetForTypeConstraint(const std::string& asset_name, const std::string& expected_type, const std::string& expected_constraint);
    
    // Type checking helpers
    bool isMovableObjectAsset(Expression* asset);
    bool isNonMovableObjectAsset(Expression* asset);
    bool isPositiveServiceAsset(Expression* asset);
    bool isNegativeServiceAsset(Expression* asset);
    bool isObjectAction(Expression* asset);
    bool isServiceAction(Expression* asset);
    
    // Pattern validation helpers
    bool isReciprocalPattern(Expression* left, Expression* right);
    std::string getDetailedJoinError(const std::string& join_type, Expression* left, Expression* right);
    
    // Type inference helpers
    std::pair<std::string, std::string> inferActionType(const std::string& action_string);
    void createImplicitActionDefinition(const std::string& action_string, const std::string& type, const std::string& constraint);


};

} // namespace witness

#endif // SEMANTIC_ANALYZER_HPP 