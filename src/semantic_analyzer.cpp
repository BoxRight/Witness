#include "semantic_analyzer.hpp"
#include <iostream>
#include <algorithm>

namespace witness {

SemanticAnalyzer::SemanticAnalyzer() {
    // Initialize the set of recognized join operations from JOINS.md
    join_operations = {
        // Universal joins
        "join", "evidence", "argument",
        
        // Object-based contextual joins
        "transfer", "sell",
        
        // Service-based contextual joins
        "compensation", "consideration", "forbearance",
        
        // Non-movable object contextual joins
        "encumber", "access", "lien"
    };
}

void SemanticAnalyzer::analyze(Program* program) {
    if (!program) {
        reportError("Cannot analyze null program");
        return;
    }
    
    // Clear previous analysis results
    errors.clear();
    warnings.clear();
    
    // Traverse all statements in the program
    for (auto& stmt : program->statements) {
        if (stmt) {
            analyzeStatement(stmt.get());
        }
    }
    
    // Report analysis results
    if (!errors.empty()) {
        std::cout << "Semantic Analysis Errors:" << std::endl;
        for (const auto& error : errors) {
            std::cout << "  Error: " << error << std::endl;
        }
    }
    
    if (!warnings.empty()) {
        std::cout << "Semantic Analysis Warnings:" << std::endl;
        for (const auto& warning : warnings) {
            std::cout << "  Warning: " << warning << std::endl;
        }
    }
}

bool SemanticAnalyzer::isJoinOperation(const std::string& function_name) const {
    return join_operations.find(function_name) != join_operations.end();
}

std::unique_ptr<Expression> SemanticAnalyzer::transformJoinCall(FunctionCallExpression* func_call) {
    if (!func_call || !func_call->function_name || !func_call->arguments) {
        return nullptr;
    }
    
    std::string join_type = func_call->function_name->name;
    
    // Validate argument count (all joins require exactly 2 arguments)
    if (func_call->arguments->expressions.size() != 2) {
        reportError("Join operation '" + join_type + "' requires exactly 2 arguments, got " + 
                   std::to_string(func_call->arguments->expressions.size()));
        return nullptr;
    }
    
    // Extract the two asset arguments
    auto left_asset = std::move(func_call->arguments->expressions[0]);
    auto right_asset = std::move(func_call->arguments->expressions[1]);
    
    // Validate the join operation semantics
    if (!validateJoinOperation(join_type, left_asset.get(), right_asset.get())) {
        reportError("Invalid join operation: " + join_type);
        return nullptr;
    }
    
    // Create and return the validated join expression
    return std::make_unique<JoinExpression>(join_type, std::move(left_asset), std::move(right_asset));
}

bool SemanticAnalyzer::validateJoinOperation(const std::string& join_type, 
                                            Expression* left_asset, 
                                            Expression* right_asset) {
    // Universal joins (no constraints)
    if (join_type == "join" || join_type == "evidence" || join_type == "argument") {
        return validateUniversalJoin(join_type, left_asset, right_asset);
    }
    
    // Contextual joins (domain-specific constraints)
    return validateContextualJoin(join_type, left_asset, right_asset);
}

void SemanticAnalyzer::reportError(const std::string& message) {
    errors.push_back(message);
}

void SemanticAnalyzer::reportWarning(const std::string& message) {
    warnings.push_back(message);
}

// Private implementation methods

void SemanticAnalyzer::analyzeStatement(Statement* stmt) {
    if (!stmt) return;
    
    // Handle different statement types
    if (auto type_def = dynamic_cast<TypeDefinition*>(stmt)) {
        analyzeTypeDefinition(type_def);
    } else if (auto asset_def = dynamic_cast<AssetDefinition*>(stmt)) {
        analyzeAssetDefinition(asset_def);
    } else if (auto clause_def = dynamic_cast<ClauseDefinition*>(stmt)) {
        analyzeClauseDefinition(clause_def);
    }
}

void SemanticAnalyzer::analyzeExpression(Expression* expr) {
    if (!expr) return;
    
    // Handle function calls - check if they're join operations
    if (auto func_call = dynamic_cast<FunctionCallExpression*>(expr)) {
        if (func_call->function_name && isJoinOperation(func_call->function_name->name)) {
            // This is a join operation - validate it
            std::string join_type = func_call->function_name->name;
            
            if (!func_call->arguments || func_call->arguments->expressions.size() != 2) {
                reportError("Join operation '" + join_type + "' requires exactly 2 arguments");
                return;
            }
            
            auto left = func_call->arguments->expressions[0].get();
            auto right = func_call->arguments->expressions[1].get();
            
            if (!validateJoinOperation(join_type, left, right)) {
                reportError("Invalid join operation: " + join_type);
            }
        }
        
        // Recursively analyze arguments
        if (func_call->arguments) {
            analyzeExpressionList(func_call->arguments.get());
        }
    }
    
    // Handle binary operations
    else if (auto binary_op = dynamic_cast<BinaryOpExpression*>(expr)) {
        analyzeExpression(binary_op->left.get());
        analyzeExpression(binary_op->right.get());
    }
    
    // Handle unary operations
    else if (auto unary_op = dynamic_cast<UnaryOpExpression*>(expr)) {
        analyzeExpression(unary_op->operand.get());
    }
}

void SemanticAnalyzer::analyzeTypeDefinition(TypeDefinition* type_def) {
    if (!type_def) return;
    
    // Analyze the properties expression list
    if (type_def->properties) {
        analyzeExpressionList(type_def->properties.get());
    }
}

void SemanticAnalyzer::analyzeAssetDefinition(AssetDefinition* asset_def) {
    if (!asset_def) return;
    
    // Analyze the asset value expression list
    if (asset_def->value) {
        analyzeExpressionList(asset_def->value.get());
    }
}

void SemanticAnalyzer::analyzeClauseDefinition(ClauseDefinition* clause_def) {
    if (!clause_def) return;
    
    // Analyze the clause expression
    if (clause_def->expression) {
        analyzeExpression(clause_def->expression.get());
    }
}

void SemanticAnalyzer::analyzeExpressionList(ExpressionList* expr_list) {
    if (!expr_list) return;
    
    for (auto& expr : expr_list->expressions) {
        analyzeExpression(expr.get());
    }
}

bool SemanticAnalyzer::validateUniversalJoin(const std::string& join_type, Expression* left, Expression* right) {
    // Universal joins have no constraints - they work on any assets
    if (!left || !right) {
        reportError("Universal join '" + join_type + "' requires two valid asset arguments");
        return false;
    }
    
    // All universal joins are valid as long as arguments exist
    return true;
}

bool SemanticAnalyzer::validateContextualJoin(const std::string& join_type, Expression* left, Expression* right) {
    // For now, implement basic validation - full type checking will come later
    if (!left || !right) {
        reportError("Contextual join '" + join_type + "' requires two valid asset arguments");
        return false;
    }
    
    // TODO: Implement specific validation for each contextual join type
    // This is where we'll add:
    // - Reciprocal pattern validation
    // - Type constraint checking (movable/non-movable, positive/negative services)
    // - Domain-specific semantic rules
    
    reportWarning("Contextual join validation not fully implemented yet: " + join_type);
    return true;
}

bool SemanticAnalyzer::validateReciprocalPattern(Expression* left, Expression* right) {
    // TODO: Implement reciprocal pattern validation
    // Check if pattern is (s1, A1, s2) and (s2, A2, s1)
    return true;
}

// Type checking helper stubs - to be implemented in later phases
bool SemanticAnalyzer::isMovableObjectAsset(Expression* asset) {
    // TODO: Implement type checking for movable objects
    return false;
}

bool SemanticAnalyzer::isNonMovableObjectAsset(Expression* asset) {
    // TODO: Implement type checking for non-movable objects
    return false;
}

bool SemanticAnalyzer::isPositiveServiceAsset(Expression* asset) {
    // TODO: Implement type checking for positive services
    return false;
}

bool SemanticAnalyzer::isNegativeServiceAsset(Expression* asset) {
    // TODO: Implement type checking for negative services
    return false;
}

bool SemanticAnalyzer::isObjectAction(Expression* asset) {
    // TODO: Implement type checking for object actions
    return false;
}

bool SemanticAnalyzer::isServiceAction(Expression* asset) {
    // TODO: Implement type checking for service actions
    return false;
}

} // namespace witness 