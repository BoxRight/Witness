#include "semantic_analyzer.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <set>

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
    
    // Initialize the set of recognized logical operations
    logical_operations.insert("oblig");
    logical_operations.insert("claim");
    logical_operations.insert("not");
    
    // Initialize the set of recognized system operations
    system_operations.insert("global");
    system_operations.insert("litis");
    system_operations.insert("meet");
    system_operations.insert("domain");
    
    // Initialize asset ID tracking for satisfiability checking
    next_asset_id = 1;
}

int SemanticAnalyzer::getOrAssignAssetID(const std::string& asset_name) {
    // Check if asset already has an ID
    auto it = asset_to_id.find(asset_name);
    if (it != asset_to_id.end()) {
        return it->second;
    }
    
    // Assign new ID and store mapping
    int new_id = next_asset_id++;
    asset_to_id[asset_name] = new_id;
    
    reportWarning("Asset '" + asset_name + "' assigned ID " + std::to_string(new_id) + " for satisfiability checking");
    
    return new_id;
}

void SemanticAnalyzer::addClause(const std::string& clause_name, const std::vector<int>& positive_literals, 
                                 const std::vector<int>& negative_literals, const std::string& expression) {
    ClauseInfo clause;
    clause.name = clause_name;
    clause.positive_literals = positive_literals;
    clause.negative_literals = negative_literals;
    clause.expression = expression;
    
    current_clauses.push_back(clause);
    
    // Debug output
    std::string pos_str = "";
    for (int lit : positive_literals) {
        pos_str += "+" + std::to_string(lit) + " ";
    }
    std::string neg_str = "";
    for (int lit : negative_literals) {
        neg_str += "-" + std::to_string(lit) + " ";
    }
    
    reportWarning("Clause '" + clause_name + "' added: [" + pos_str + neg_str + "] from '" + expression + "'");
    // Print the per-clause truth table
    printClauseTruthTable(current_clauses.back());
}

SemanticAnalyzer::SatisfiabilityResult SemanticAnalyzer::generateTruthTable() {
    SatisfiabilityResult result;
    result.satisfiable = false;
    
    if (current_clauses.empty()) {
        result.satisfiable = true;
        result.assignments.push_back({}); // Empty assignment satisfies no clauses
        return result;
    }
    
    // Collect all unique asset IDs used in clauses
    std::set<int> all_asset_ids;
    for (const auto& clause : current_clauses) {
        for (int lit : clause.positive_literals) {
            all_asset_ids.insert(lit);
        }
        for (int lit : clause.negative_literals) {
            all_asset_ids.insert(lit);
        }
    }
    
    std::vector<int> asset_ids(all_asset_ids.begin(), all_asset_ids.end());
    int num_assets = asset_ids.size();
    
    reportWarning("Truth table generation: " + std::to_string(num_assets) + " assets, " + 
                  std::to_string(current_clauses.size()) + " clauses, " + 
                  std::to_string(1 << num_assets) + " combinations to check");
    
    // Generate all possible 2^n truth assignments
    for (int assignment = 0; assignment < (1 << num_assets); assignment++) {
        std::vector<int> current_assignment;
        
        // Create signed literal assignment
        for (int i = 0; i < num_assets; i++) {
            int asset_id = asset_ids[i];
            if (assignment & (1 << i)) {
                current_assignment.push_back(asset_id);  // Positive literal
            } else {
                current_assignment.push_back(-asset_id); // Negative literal
            }
        }
        
        // Check if this assignment satisfies all clauses
        bool satisfies_all = true;
        for (const auto& clause : current_clauses) {
            bool clause_satisfied = false;
            
            // Check positive literals
            for (int pos_lit : clause.positive_literals) {
                for (int assigned_lit : current_assignment) {
                    if (assigned_lit == pos_lit) {
                        clause_satisfied = true;
                        break;
                    }
                }
                if (clause_satisfied) break;
            }
            
            // Check negative literals
            if (!clause_satisfied) {
                for (int neg_lit : clause.negative_literals) {
                    for (int assigned_lit : current_assignment) {
                        if (assigned_lit == -neg_lit) {
                            clause_satisfied = true;
                            break;
                        }
                    }
                    if (clause_satisfied) break;
                }
            }
            
            if (!clause_satisfied) {
                satisfies_all = false;
                break;
            }
        }
        
        if (satisfies_all) {
            result.assignments.push_back(current_assignment);
        }
    }
    
    result.satisfiable = !result.assignments.empty();
    
    if (result.satisfiable) {
        reportWarning("Truth table generation completed: " + std::to_string(result.assignments.size()) + " satisfying assignments found");
    } else {
        result.error_message = "No satisfying assignments found - clauses are unsatisfiable";
        reportError(result.error_message);
    }
    
    return result;
}

void SemanticAnalyzer::analyze(Program* program) {
    if (!program) {
        reportError("Cannot analyze null program");
        return;
    }
    
    // Clear previous analysis results
    errors.clear();
    warnings.clear();
    symbol_table.clear();
    
    // First pass: Build symbol table from type definitions
    for (auto& stmt : program->statements) {
        if (stmt) {
            if (auto type_def = dynamic_cast<TypeDefinition*>(stmt.get())) {
                registerTypeDefinition(type_def);
            }
        }
    }
    
    // Second pass: Register asset definitions in symbol table
    for (auto& stmt : program->statements) {
        if (stmt) {
            if (auto asset_def = dynamic_cast<AssetDefinition*>(stmt.get())) {
                registerAssetDefinition(asset_def);
            }
        }
    }
    
    // Third pass: Analyze all statements with full type information
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
    
    // Report completion status
    if (errors.empty()) {
        std::cout << "Semantic analysis completed successfully!" << std::endl;
        std::cout << "- System operations validated: global(), domain(), litis(), meet()" << std::endl;
        std::cout << "- Join operations validated: transfer, sell, compensation, consideration, forbearance, encumber" << std::endl;
        std::cout << "- Logical operations validated: oblig(), claim(), not()" << std::endl;
    } else {
        std::cout << "Semantic analysis completed with " << errors.size() << " error(s)" << std::endl;
    }
}

void SemanticAnalyzer::registerTypeDefinition(TypeDefinition* type_def) {
    if (!type_def || !type_def->name) return;
    
    std::string name = type_def->name->name;
    std::string constraint = type_def->getConstraint();
    
    // For actions, we need to store their components to trace the type chain
    if (type_def->type_keyword == "action" && type_def->properties) {
        std::vector<std::string> components;
        
        // Extract action components (description, referenced type)
        for (const auto& expr : type_def->properties->expressions) {
            if (auto identifier = dynamic_cast<Identifier*>(expr.get())) {
                components.push_back(identifier->name);
            } else if (auto string_literal = dynamic_cast<StringLiteral*>(expr.get())) {
                components.push_back(string_literal->value);
            }
        }
        
        symbol_table[name] = TypeInfo(type_def->type_keyword, constraint, components);
    } else {
        symbol_table[name] = TypeInfo(type_def->type_keyword, constraint);
    }
}

void SemanticAnalyzer::registerAssetDefinition(AssetDefinition* asset_def) {
    if (!asset_def || !asset_def->name || !asset_def->value) return;
    
    std::string name = asset_def->name->name;
    std::vector<std::string> components;
    
    // Extract asset components from the expression list
    if (asset_def->value) {
        for (size_t i = 0; i < asset_def->value->expressions.size(); ++i) {
            const auto& expr = asset_def->value->expressions[i];
            
            if (auto identifier = dynamic_cast<Identifier*>(expr.get())) {
                components.push_back(identifier->name);
            } else if (auto string_literal = dynamic_cast<StringLiteral*>(expr.get())) {
                components.push_back(string_literal->value);
                
                // Type inference: if this is a string literal in the action position (index 1),
                // try to infer its type and create an implicit action definition
                if (i == 1) { // Action position in asset definition [subject, action, object]
                    auto inferred_type = inferActionType(string_literal->value);
                    createImplicitActionDefinition(string_literal->value, inferred_type.first, inferred_type.second);
                    
                    // Report the inference as a warning so users know what happened
                    reportWarning("Type inference: action '" + string_literal->value + 
                                 "' inferred as " + inferred_type.first + " (" + inferred_type.second + ")");
                }
            }
        }
    }
    
    // Store the asset with its components
    symbol_table[name] = TypeInfo("asset", "", components);
}

TypeInfo* SemanticAnalyzer::lookupType(const std::string& identifier) {
    auto it = symbol_table.find(identifier);
    if (it != symbol_table.end()) {
        return &it->second;
    }
    return nullptr;
}

bool SemanticAnalyzer::isJoinOperation(const std::string& function_name) const {
    return join_operations.find(function_name) != join_operations.end();
}

bool SemanticAnalyzer::isLogicalOperation(const std::string& function_name) const {
    return logical_operations.find(function_name) != logical_operations.end();
}

bool SemanticAnalyzer::isSystemOperation(const std::string& function_name) const {
    return system_operations.find(function_name) != system_operations.end();
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

bool SemanticAnalyzer::checkIdempotency(const std::string& join_type, Expression* left, Expression* right) {
    // Check for idempotency: join(x, x) = x
    if (auto left_id = dynamic_cast<Identifier*>(left)) {
        if (auto right_id = dynamic_cast<Identifier*>(right)) {
            if (left_id->name == right_id->name) {
                reportWarning("Idempotent " + join_type + " operation: " + join_type + "(" + 
                             left_id->name + ", " + left_id->name + ") = " + left_id->name);
                return true;
            }
        }
    }
    return false;
}

bool SemanticAnalyzer::validateJoinOperation(const std::string& join_type, 
                                            Expression* left_asset, 
                                            Expression* right_asset) {
    // First check for idempotency - applies to ALL join operations
    if (checkIdempotency(join_type, left_asset, right_asset)) {
        return true; // Idempotent operations are always valid
    }
    
    // Universal joins (no constraints)
    if (join_type == "join" || join_type == "evidence" || join_type == "argument") {
        return validateUniversalJoin(join_type, left_asset, right_asset);
    }
    
    // Contextual joins with specific validation
    if (join_type == "transfer") return validateTransferJoin(left_asset, right_asset);
    if (join_type == "sell") return validateSellJoin(left_asset, right_asset);
    if (join_type == "compensation") return validateCompensationJoin(left_asset, right_asset);
    if (join_type == "consideration") return validateConsiderationJoin(left_asset, right_asset);
    if (join_type == "forbearance") return validateForbearanceJoin(left_asset, right_asset);
    if (join_type == "encumber") return validateEncumberJoin(left_asset, right_asset);
    if (join_type == "access") return validateAccessJoin(left_asset, right_asset);
    if (join_type == "lien") return validateLienJoin(left_asset, right_asset);
    
    return validateContextualJoin(join_type, left_asset, right_asset);
}

bool SemanticAnalyzer::validateLogicalOperation(const std::string& operation_type, FunctionCallExpression* func_call) {
    if (!func_call) {
        reportError("Logical operation requires a valid function call");
        return false;
    }
    
    // Dispatch to specific validators
    if (operation_type == "oblig") {
        return validateObligOperation(func_call);
    } else if (operation_type == "claim") {
        return validateClaimOperation(func_call);
    } else if (operation_type == "not") {
        return validateNotOperation(func_call);
    }
    
    reportError("Unknown logical operation: " + operation_type);
    return false;
}

bool SemanticAnalyzer::validateSystemOperation(const std::string& operation_type, FunctionCallExpression* func_call) {
    if (!func_call) {
        reportError("System operation requires a valid function call");
        return false;
    }
    
    // Dispatch to specific validators
    if (operation_type == "global") {
        return validateGlobalOperation(func_call);
    } else if (operation_type == "litis") {
        return validateLitisOperation(func_call);
    } else if (operation_type == "meet") {
        return validateMeetOperation(func_call);
    } else if (operation_type == "domain") {
        return validateDomainOperation(func_call);
    }
    
    reportError("Unknown system operation: " + operation_type);
    return false;
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
        if (func_call->function_name) {
            std::string function_name = func_call->function_name->name;
            
            // Check if it's a join operation
            if (isJoinOperation(function_name)) {
                // This is a join operation - validate it
            if (!func_call->arguments || func_call->arguments->expressions.size() != 2) {
                    reportError("Join operation '" + function_name + "' requires exactly 2 arguments");
                return;
            }
            
            auto left = func_call->arguments->expressions[0].get();
            auto right = func_call->arguments->expressions[1].get();
            
                if (!validateJoinOperation(function_name, left, right)) {
                    reportError(getDetailedJoinError(function_name, left, right));
                }
            }
            // Check if it's a logical operation
            else if (isLogicalOperation(function_name)) {
                // This is a logical operation - validate it
                if (!validateLogicalOperation(function_name, func_call)) {
                    reportError("Logical operation '" + function_name + "' validation failed");
                }
            }
            // Check if it's a system operation
            else if (isSystemOperation(function_name)) {
                // This is a system operation - validate it
                if (!validateSystemOperation(function_name, func_call)) {
                    reportError("System operation '" + function_name + "' validation failed");
                }
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
    
    // Analyze the clause expression and collect clauses for satisfiability checking
    if (clause_def->expression) {
        std::string clause_name = clause_def->name ? clause_def->name->name : "unnamed_clause";
        analyzeClauseExpression(clause_def->expression.get(), clause_name);
    }
}

void SemanticAnalyzer::analyzeClauseExpression(Expression* expr, const std::string& clause_name) {
    if (!expr) return;
    
    // Handle function calls (logical operations)
    if (auto func_call = dynamic_cast<FunctionCallExpression*>(expr)) {
        std::string function_name = func_call->function_name->name;
        
        if (isLogicalOperation(function_name)) {
            // Handle logical operations by creating clauses
            if (function_name == "oblig" || function_name == "claim") {
                // Both oblig and claim create positive literals
                if (func_call->arguments && func_call->arguments->expressions.size() == 1) {
                    auto arg = func_call->arguments->expressions[0].get();
                    if (auto identifier = dynamic_cast<Identifier*>(arg)) {
                        int asset_id = getOrAssignAssetID(identifier->name);
                        addClause(clause_name, {asset_id}, {}, function_name + "(" + identifier->name + ")");
                    }
                }
            } else if (function_name == "not") {
                // not() creates negative literals
                if (func_call->arguments && func_call->arguments->expressions.size() == 1) {
                    auto arg = func_call->arguments->expressions[0].get();
                    if (auto identifier = dynamic_cast<Identifier*>(arg)) {
                        int asset_id = getOrAssignAssetID(identifier->name);
                        addClause(clause_name, {}, {asset_id}, "not(" + identifier->name + ")");
                    }
                }
            }
            
            // Still validate the operation
            validateLogicalOperation(function_name, func_call);
        } else {
            // Not a logical operation, analyze normally
            analyzeExpression(expr);
        }
    } else {
        // Not a function call, analyze normally
        analyzeExpression(expr);
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
    // Fallback for unknown contextual joins
    if (!left || !right) {
        reportError("Contextual join '" + join_type + "' requires two valid asset arguments");
        return false;
    }
    
    reportWarning("Unknown contextual join type: " + join_type);
    return true;
}

// Specific contextual join validators

bool SemanticAnalyzer::validateTransferJoin(Expression* left, Expression* right) {
    // transfer: movable object ↔ movable object (reciprocal pattern)
    if (!isReciprocalPattern(left, right)) {
        reportError("transfer operation requires reciprocal pattern: (s1,A1,s2) ↔ (s2,A2,s1)");
        return false;
    }
    
    if (!isMovableObjectAsset(left) || !isMovableObjectAsset(right)) {
        reportError("transfer operation requires both assets to involve movable objects");
        return false;
    }
    
    return true;
}

bool SemanticAnalyzer::validateSellJoin(Expression* left, Expression* right) {
    // sell: object action ↔ positive service action (reciprocal pattern)
    if (!isReciprocalPattern(left, right)) {
        reportError("sell operation requires reciprocal pattern: (s1,A1,s2) ↔ (s2,A2,s1)");
        return false;
    }
    
    if (!isObjectAction(left) || !isPositiveServiceAsset(right)) {
        reportError("sell operation requires object action ↔ positive service action");
        return false;
    }
    
    return true;
}

bool SemanticAnalyzer::validateCompensationJoin(Expression* left, Expression* right) {
    // compensation: positive service ↔ positive service (reciprocal pattern)
    if (!isReciprocalPattern(left, right)) {
        reportError("compensation operation requires reciprocal pattern: (s1,A1,s2) ↔ (s2,A2,s1)");
        return false;
    }
    
    if (!isPositiveServiceAsset(left) || !isPositiveServiceAsset(right)) {
        reportError("compensation operation requires both assets to involve positive services");
        return false;
    }
    
    return true;
}

bool SemanticAnalyzer::validateConsiderationJoin(Expression* left, Expression* right) {
    // consideration: positive service ↔ negative service (reciprocal pattern)
    if (!isReciprocalPattern(left, right)) {
        reportError("consideration operation requires reciprocal pattern: (s1,A1,s2) ↔ (s2,A2,s1)");
        return false;
    }
    
    if (!isPositiveServiceAsset(left) || !isNegativeServiceAsset(right)) {
        reportError("consideration operation requires positive service ↔ negative service");
        return false;
    }
    
    return true;
}

bool SemanticAnalyzer::validateForbearanceJoin(Expression* left, Expression* right) {
    // forbearance: negative service ↔ negative service (reciprocal pattern)
    if (!isReciprocalPattern(left, right)) {
        reportError("forbearance operation requires reciprocal pattern: (s1,A1,s2) ↔ (s2,A2,s1)");
        return false;
    }
    
    if (!isNegativeServiceAsset(left) || !isNegativeServiceAsset(right)) {
        reportError("forbearance operation requires both assets to involve negative services");
        return false;
    }
    
    return true;
}

bool SemanticAnalyzer::validateEncumberJoin(Expression* left, Expression* right) {
    // encumber: non-movable object ↔ positive service (reciprocal pattern)
    if (!isReciprocalPattern(left, right)) {
        reportError("encumber operation requires reciprocal pattern: (s1,A1,s2) ↔ (s2,A2,s1)");
        return false;
    }
    
    if (!isNonMovableObjectAsset(left) || !isPositiveServiceAsset(right)) {
        reportError("encumber operation requires non-movable object ↔ positive service");
        return false;
    }
    
    return true;
}

bool SemanticAnalyzer::validateAccessJoin(Expression* left, Expression* right) {
    // access: non-movable object ↔ positive service (reciprocal pattern)
    if (!isReciprocalPattern(left, right)) {
        reportError("access operation requires reciprocal pattern: (s1,A1,s2) ↔ (s2,A2,s1)");
        return false;
    }
    
    if (!isNonMovableObjectAsset(left) || !isPositiveServiceAsset(right)) {
        reportError("access operation requires non-movable object ↔ positive service");
        return false;
    }
    
    return true;
}

bool SemanticAnalyzer::validateLienJoin(Expression* left, Expression* right) {
    // lien: non-movable object ↔ negative service (reciprocal pattern)
    if (!isReciprocalPattern(left, right)) {
        reportError("lien operation requires reciprocal pattern: (s1,A1,s2) ↔ (s2,A2,s1)");
        return false;
    }
    
    if (!isNonMovableObjectAsset(left) || !isNegativeServiceAsset(right)) {
        reportError("lien operation requires non-movable object ↔ negative service");
        return false;
    }
    
    return true;
}

// Asset analysis helpers

bool SemanticAnalyzer::isAssetExpression(Expression* expr) {
    // An asset is typically represented as an expression list with 3 components: subject, action, object
    if (auto identifier = dynamic_cast<Identifier*>(expr)) {
        // Check if this identifier refers to a defined asset
        return lookupType(identifier->name) != nullptr;
    }
    return false;
}

std::vector<std::string> SemanticAnalyzer::getAssetComponents(Expression* asset) {
    std::vector<std::string> components;
    
    if (auto identifier = dynamic_cast<Identifier*>(asset)) {
        // Look up the asset in the symbol table
        TypeInfo* asset_info = lookupType(identifier->name);
        if (asset_info && asset_info->type_keyword == "asset") {
            return asset_info->asset_components;
        }
    }
    
    // If it's an expression list, extract components directly
    // This would be for inline asset definitions like: alice, "sell", bob
    if (auto expr_list = dynamic_cast<ExpressionList*>(asset)) {
        for (const auto& expr : expr_list->expressions) {
            if (auto id = dynamic_cast<Identifier*>(expr.get())) {
                components.push_back(id->name);
            } else if (auto str_lit = dynamic_cast<StringLiteral*>(expr.get())) {
                components.push_back(str_lit->value);
            }
        }
    }
    
    return components;
}

std::string SemanticAnalyzer::getAssetSubject(Expression* asset) {
    auto components = getAssetComponents(asset);
    return components.size() > 0 ? components[0] : "";
}

std::string SemanticAnalyzer::getAssetAction(Expression* asset) {
    auto components = getAssetComponents(asset);
    return components.size() > 1 ? components[1] : "";
}

std::string SemanticAnalyzer::getAssetObject(Expression* asset) {
    auto components = getAssetComponents(asset);
    return components.size() > 2 ? components[2] : "";
}

// Type checking helpers

bool SemanticAnalyzer::isMovableObjectAsset(Expression* asset) {
    if (auto identifier = dynamic_cast<Identifier*>(asset)) {
        return analyzeAssetTypeConstraint(identifier->name, "object", "movable");
    }
    return false;
}

bool SemanticAnalyzer::isNonMovableObjectAsset(Expression* asset) {
    if (auto identifier = dynamic_cast<Identifier*>(asset)) {
        return analyzeAssetTypeConstraint(identifier->name, "object", "non_movable");
    }
    return false;
}

bool SemanticAnalyzer::isPositiveServiceAsset(Expression* asset) {
    if (auto identifier = dynamic_cast<Identifier*>(asset)) {
        return analyzeAssetTypeConstraint(identifier->name, "service", "positive");
    }
    return false;
}

bool SemanticAnalyzer::isNegativeServiceAsset(Expression* asset) {
    if (auto identifier = dynamic_cast<Identifier*>(asset)) {
        return analyzeAssetTypeConstraint(identifier->name, "service", "negative");
    }
    return false;
}

bool SemanticAnalyzer::isObjectAction(Expression* asset) {
    return isMovableObjectAsset(asset) || isNonMovableObjectAsset(asset);
}

bool SemanticAnalyzer::isServiceAction(Expression* asset) {
    return isPositiveServiceAsset(asset) || isNegativeServiceAsset(asset);
}

// Asset analysis helpers for determining asset types through type system

bool SemanticAnalyzer::analyzeAssetTypeConstraint(const std::string& asset_name, const std::string& expected_type, const std::string& expected_constraint) {
    // Look up the asset in the symbol table
    TypeInfo* asset_info = lookupType(asset_name);
    if (!asset_info) {
        return false;
    }
    
    // If it's directly a type with the expected constraint (not an asset)
    if (asset_info->type_keyword == expected_type && asset_info->constraint == expected_constraint) {
        return true;
    }
    
    // If it's an asset, we need to analyze its action to determine the underlying type
    if (asset_info->type_keyword == "asset") {
        return analyzeAssetForTypeConstraint(asset_name, expected_type, expected_constraint);
    }
    
    return false;
}

// Remove the old heuristic-based methods
bool SemanticAnalyzer::analyzeAssetForMovableObject(const std::string& asset_name) {
    return analyzeAssetForTypeConstraint(asset_name, "object", "movable");
}

bool SemanticAnalyzer::analyzeAssetForNonMovableObject(const std::string& asset_name) {
    return analyzeAssetForTypeConstraint(asset_name, "object", "non_movable");
}

bool SemanticAnalyzer::analyzeAssetForPositiveService(const std::string& asset_name) {
    return analyzeAssetForTypeConstraint(asset_name, "service", "positive");
}

bool SemanticAnalyzer::analyzeAssetForNegativeService(const std::string& asset_name) {
    return analyzeAssetForTypeConstraint(asset_name, "service", "negative");
}

// Pattern validation helpers

bool SemanticAnalyzer::isReciprocalPattern(Expression* left, Expression* right) {
    if (!left || !right) {
        return false;
    }
    
    // Get asset components [subject, action, object]
    auto left_components = getAssetComponents(left);
    auto right_components = getAssetComponents(right);
    
    // Both assets must have at least 3 components (subject, action, object)
    if (left_components.size() < 3 || right_components.size() < 3) {
        return false;
    }
    
    // Check if pattern is (s1,A1,s2) ↔ (s2,A2,s1)
    // Left:  [s1, A1, s2]
    // Right: [s2, A2, s1]
    // So: left[0] == right[2] AND left[2] == right[0]
    bool subjects_swapped = (left_components[0] == right_components[2] && 
                            left_components[2] == right_components[0]);
    

    
    return subjects_swapped;
}

bool SemanticAnalyzer::validateReciprocalPattern(Expression* left, Expression* right) {
    return isReciprocalPattern(left, right);
}

std::string SemanticAnalyzer::getDetailedJoinError(const std::string& join_type, Expression* left, Expression* right) {
    std::string error = "Join operation '" + join_type + "' failed:\n";
    
    // Get asset components for detailed analysis
    auto left_components = getAssetComponents(left);
    auto right_components = getAssetComponents(right);
    
    if (auto left_id = dynamic_cast<Identifier*>(left)) {
        error += "  Left asset: " + left_id->name;
        if (left_components.size() >= 3) {
            error += " = (" + left_components[0] + ", " + left_components[1] + ", " + left_components[2] + ")";
        }
        error += "\n";
    }
    
    if (auto right_id = dynamic_cast<Identifier*>(right)) {
        error += "  Right asset: " + right_id->name;
        if (right_components.size() >= 3) {
            error += " = (" + right_components[0] + ", " + right_components[1] + ", " + right_components[2] + ")";
        }
        error += "\n";
    }
    
    // Check reciprocal pattern for contextual joins
    if (join_type != "join" && join_type != "evidence" && join_type != "argument") {
        if (left_components.size() >= 3 && right_components.size() >= 3) {
            bool is_reciprocal = (left_components[0] == right_components[2] && 
                                 left_components[2] == right_components[0]);
            error += "  Reciprocal pattern: " + std::string(is_reciprocal ? "VALID" : "INVALID");
            if (!is_reciprocal) {
                error += " (Expected: " + left_components[0] + " ↔ " + right_components[2] + 
                        " and " + left_components[2] + " ↔ " + right_components[0] + ")";
            }
            error += "\n";
        }
    }
    
    return error;
}

// Type system-based asset analysis - requires proper action definitions
bool SemanticAnalyzer::analyzeAssetForTypeConstraint(const std::string& asset_name, const std::string& expected_type, const std::string& expected_constraint) {
    // Look up the asset in the symbol table
    TypeInfo* asset_info = lookupType(asset_name);
    if (!asset_info || asset_info->type_keyword != "asset") {
        return false;
    }
    
    // An asset should have components [subject, action, object]
    if (asset_info->asset_components.size() < 2) {
        return false;
    }
    
    // The action is typically the second component
    std::string action_name = asset_info->asset_components[1];
    
    // Look up the action in the symbol table
    TypeInfo* action_info = lookupType(action_name);
    if (!action_info || action_info->type_keyword != "action") {
        return false;
    }
    
    // Actions should have components [description, referenced_type]
    if (action_info->asset_components.size() < 2) {
        return false;
    }
    
    // The referenced type is typically the second component of the action
    std::string referenced_type_name = action_info->asset_components[1];
    
    // Look up the referenced type
    TypeInfo* referenced_type_info = lookupType(referenced_type_name);
    if (!referenced_type_info) {
        return false;
    }
    
    // Check if the referenced type matches the expected type and constraint
    return (referenced_type_info->type_keyword == expected_type && 
            referenced_type_info->constraint == expected_constraint);
}

// Logical operation validation helpers

bool SemanticAnalyzer::validateObligOperation(FunctionCallExpression* func_call) {
    if (!func_call || !func_call->arguments) {
        reportError("oblig() operation requires an argument");
        return false;
    }
    
    // oblig() should take exactly one argument (an asset or expression)
    if (func_call->arguments->expressions.size() != 1) {
        reportError("oblig() operation requires exactly 1 argument, got " + 
                   std::to_string(func_call->arguments->expressions.size()));
        return false;
    }
    
    // The argument should be a valid asset identifier or expression
    auto arg = func_call->arguments->expressions[0].get();
    if (!arg) {
        reportError("oblig() operation requires a valid argument");
        return false;
    }
    
    // If the argument is an asset identifier, assign it an ID for satisfiability checking
    if (auto identifier = dynamic_cast<Identifier*>(arg)) {
        int asset_id = getOrAssignAssetID(identifier->name);
        reportWarning("oblig(" + identifier->name + ") - asset ID " + std::to_string(asset_id) + " marked as positive literal");
    }
    
    // For now, we allow any expression as argument to oblig()
    // TODO: Could add more specific validation for asset types
    return true;
}

bool SemanticAnalyzer::validateClaimOperation(FunctionCallExpression* func_call) {
    if (!func_call || !func_call->arguments) {
        reportError("claim() operation requires an argument");
        return false;
    }
    
    // claim() should take exactly one argument (an asset or expression)
    if (func_call->arguments->expressions.size() != 1) {
        reportError("claim() operation requires exactly 1 argument, got " + 
                   std::to_string(func_call->arguments->expressions.size()));
        return false;
    }
    
    // The argument should be a valid asset identifier or expression
    auto arg = func_call->arguments->expressions[0].get();
    if (!arg) {
        reportError("claim() operation requires a valid argument");
        return false;
    }
    
    // If the argument is an asset identifier, assign it an ID for satisfiability checking
    if (auto identifier = dynamic_cast<Identifier*>(arg)) {
        int asset_id = getOrAssignAssetID(identifier->name);
        reportWarning("claim(" + identifier->name + ") - asset ID " + std::to_string(asset_id) + " marked as positive literal");
    }
    
    // For now, we allow any expression as argument to claim()
    // TODO: Could add more specific validation for asset types
    return true;
}

bool SemanticAnalyzer::validateNotOperation(FunctionCallExpression* func_call) {
    if (!func_call || !func_call->arguments) {
        reportError("not() operation requires an argument");
        return false;
    }
    
    // not() should take exactly one argument (a boolean expression)
    if (func_call->arguments->expressions.size() != 1) {
        reportError("not() operation requires exactly 1 argument, got " + 
                   std::to_string(func_call->arguments->expressions.size()));
        return false;
    }
    
    // The argument should be a valid boolean expression
    auto arg = func_call->arguments->expressions[0].get();
    if (!arg) {
        reportError("not() operation requires a valid argument");
        return false;
    }
    
    // If the argument is an asset identifier, assign it an ID for satisfiability checking
    if (auto identifier = dynamic_cast<Identifier*>(arg)) {
        int asset_id = getOrAssignAssetID(identifier->name);
        reportWarning("not(" + identifier->name + ") - asset ID " + std::to_string(asset_id) + " marked as negative literal");
    }
    
    // For now, we allow any expression as argument to not()
    // TODO: Could add more specific validation for boolean expressions
    return true;
}

// System operation validation helpers

bool SemanticAnalyzer::validateGlobalOperation(FunctionCallExpression* func_call) {
    if (!func_call || !func_call->arguments) {
        reportError("global() operation requires an argument list");
        return false;
    }
    
    // global() should take no arguments
    if (func_call->arguments->expressions.size() != 0) {
        reportError("global() operation requires no arguments, got " + 
                   std::to_string(func_call->arguments->expressions.size()));
        return false;
    }
    
    // Trigger truth table generation for satisfiability checking
    reportWarning("global() operation triggered - generating truth table...");
    
    SatisfiabilityResult result = generateTruthTable();
    
    if (result.satisfiable) {
        reportWarning("global() operation successful - system is satisfiable");
        
        // Report satisfying assignments
        for (size_t i = 0; i < result.assignments.size(); i++) {
            std::string assignment_str = "Assignment " + std::to_string(i + 1) + ": [";
            for (size_t j = 0; j < result.assignments[i].size(); j++) {
                int lit = result.assignments[i][j];
                if (lit > 0) {
                    assignment_str += "+" + std::to_string(lit);
                } else {
                    assignment_str += std::to_string(lit);
                }
                if (j < result.assignments[i].size() - 1) {
                    assignment_str += ", ";
                }
            }
            assignment_str += "]";
            reportWarning(assignment_str);
        }
    } else {
        reportError("global() operation failed - system is unsatisfiable: " + result.error_message);
        return false;
    }
    
    return true;
}

bool SemanticAnalyzer::validateLitisOperation(FunctionCallExpression* func_call) {
    if (!func_call || !func_call->arguments) {
        reportError("litis() operation requires an argument list");
        return false;
    }
    
    // litis() should take at least one argument (asset identifiers)
    if (func_call->arguments->expressions.size() < 1) {
        reportError("litis() operation requires at least 1 argument, got " + 
                   std::to_string(func_call->arguments->expressions.size()));
        return false;
    }
    
    // All arguments should be valid asset identifiers
    for (const auto& arg : func_call->arguments->expressions) {
        if (!arg) {
            reportError("litis() operation requires valid asset arguments");
            return false;
        }
    }
    
    // For now, all litis() operations are valid
    // TODO: Implement selective satisfiability checking for specified assets
    return true;
}

bool SemanticAnalyzer::validateMeetOperation(FunctionCallExpression* func_call) {
    if (!func_call || !func_call->arguments) {
        reportError("meet() operation requires an argument list");
        return false;
    }
    
    // meet() should take exactly two arguments (joined assets)
    if (func_call->arguments->expressions.size() != 2) {
        reportError("meet() operation requires exactly 2 arguments, got " + 
                   std::to_string(func_call->arguments->expressions.size()));
        return false;
    }
    
    // Both arguments should be valid asset identifiers
    auto left_arg = func_call->arguments->expressions[0].get();
    auto right_arg = func_call->arguments->expressions[1].get();
    
    if (!left_arg || !right_arg) {
        reportError("meet() operation requires valid asset arguments");
        return false;
    }
    
    // For now, all meet() operations are valid
    // TODO: Implement validation that arguments are results of join operations
    // TODO: Implement greatest common legal denominator extraction
    return true;
}

bool SemanticAnalyzer::validateDomainOperation(FunctionCallExpression* func_call) {
    if (!func_call || !func_call->arguments) {
        reportError("domain() operation requires an argument list");
        return false;
    }
    
    // domain() should take at least one argument (asset identifiers)
    if (func_call->arguments->expressions.size() < 1) {
        reportError("domain() operation requires at least 1 argument, got " + 
                   std::to_string(func_call->arguments->expressions.size()));
        return false;
    }
    
    // All arguments should be valid asset identifiers
    for (const auto& arg : func_call->arguments->expressions) {
        if (!arg) {
            reportError("domain() operation requires valid asset arguments");
            return false;
        }
    }
    
    // For now, all domain() operations are valid
    // TODO: Implement domain analysis functionality
    return true;
}

// Type inference helpers

std::pair<std::string, std::string> SemanticAnalyzer::inferActionType(const std::string& action_string) {
    // Convert to lowercase for pattern matching
    std::string lower_action = action_string;
    std::transform(lower_action.begin(), lower_action.end(), lower_action.begin(), ::tolower);
    
    // Define common action patterns and their inferred types
    // Format: {pattern, {type, constraint}}
    
    // Positive service actions (payment, provision, delivery)
    std::vector<std::string> positive_service_patterns = {
        "pay", "charge", "bill", "invoice", "compensate", "remunerate", "salary", "wage",
        "provide", "supply", "deliver", "give", "offer", "grant", "award", "bestow",
        "serve", "assist", "help", "support", "maintain", "care", "tend", "feed",
        "repair", "fix", "restore", "renovate", "improve", "enhance", "upgrade",
        "teach", "train", "educate", "inform", "advise", "counsel", "guide", "direct"
    };
    
    // Negative service actions (restrictions, obligations, prohibitions)
    std::vector<std::string> negative_service_patterns = {
        "forbid", "prohibit", "ban", "restrict", "limit", "constrain", "confine",
        "abstain", "refrain", "avoid", "prevent", "stop", "cease", "desist",
        "obligation", "duty", "requirement", "compulsion", "mandate", "impose",
        "burden", "encumber", "bind", "tie", "commit", "pledge", "vow",
        "silence", "secrecy", "confidentiality", "nondisclosure", "privacy"
    };
    
    // Movable object actions (transfer, possession, handling)
    std::vector<std::string> movable_object_patterns = {
        "transfer", "convey", "transport", "move", "shift", "carry", "bear",
        "sell", "buy", "purchase", "acquire", "obtain", "get", "receive",
        "exchange", "trade", "swap", "barter", "negotiate", "deal",
        "lend", "loan", "borrow", "rent", "lease", "hire", "charter",
        "deliver", "ship", "send", "mail", "post", "dispatch", "forward",
        "hand", "pass", "transmit", "relay", "convey", "communicate"
    };
    
    // Non-movable object actions (real estate, property, location)
    std::vector<std::string> non_movable_object_patterns = {
        "own", "possess", "hold", "have", "control", "command", "dominate",
        "occupy", "inhabit", "dwell", "reside", "live", "stay", "remain",
        "build", "construct", "erect", "establish", "found", "create",
        "demolish", "destroy", "tear", "raze", "level", "flatten",
        "register", "record", "inscribe", "enroll", "list", "catalog",
        "mortgage", "lien", "encumber", "secure", "guarantee", "pledge"
    };
    
    // Check each pattern category
    for (const auto& pattern : positive_service_patterns) {
        if (lower_action.find(pattern) != std::string::npos) {
            return {"service", "positive"};
        }
    }
    
    for (const auto& pattern : negative_service_patterns) {
        if (lower_action.find(pattern) != std::string::npos) {
            return {"service", "negative"};
        }
    }
    
    for (const auto& pattern : movable_object_patterns) {
        if (lower_action.find(pattern) != std::string::npos) {
            return {"object", "movable"};
        }
    }
    
    for (const auto& pattern : non_movable_object_patterns) {
        if (lower_action.find(pattern) != std::string::npos) {
            return {"object", "non_movable"};
        }
    }
    
    // Default inference: actions involving money/payment typically relate to services
    if (lower_action.find("price") != std::string::npos || 
        lower_action.find("cost") != std::string::npos ||
        lower_action.find("fee") != std::string::npos ||
        lower_action.find("tax") != std::string::npos ||
        lower_action.find("interest") != std::string::npos) {
        return {"service", "positive"};
    }
    
    // Default inference: assume movable object for unknown actions
    // This is a reasonable default since most actions involve transferable items
    return {"object", "movable"};
}

void SemanticAnalyzer::createImplicitActionDefinition(const std::string& action_string, const std::string& type, const std::string& constraint) {
    // Create a synthetic action name based on the string
    std::string action_name = "inferred_" + action_string;
    
    // Check if this action already exists
    if (lookupType(action_name) != nullptr) {
        return; // Already defined
    }
    
    // Create a synthetic type name for the referenced type
    std::string type_name = "inferred_" + type + "_" + constraint;
    
    // Create the referenced type if it doesn't exist
    if (lookupType(type_name) == nullptr) {
        symbol_table[type_name] = TypeInfo(type, constraint);
    }
    
    // Create the action definition
    // Actions have components: [description, referenced_type]
    std::vector<std::string> action_components = {action_string, type_name};
    symbol_table[action_name] = TypeInfo("action", "", action_components);
    
    // Also create a mapping from the original string to the action name
    // This allows the type checking to work with the original string
    symbol_table[action_string] = TypeInfo("action", "", action_components);
}

void SemanticAnalyzer::printClauseTruthTable(const ClauseInfo& clause) {
    // Collect all asset IDs used in this clause
    std::vector<int> asset_ids;
    for (int id : clause.positive_literals) asset_ids.push_back(id);
    for (int id : clause.negative_literals) asset_ids.push_back(id);
    // Remove duplicates
    std::sort(asset_ids.begin(), asset_ids.end());
    asset_ids.erase(std::unique(asset_ids.begin(), asset_ids.end()), asset_ids.end());
    int n = asset_ids.size();
    if (n == 0) {
        std::cout << "Clause '" << clause.name << "' has no asset variables.\n";
        return;
    }
    // Print header
    std::cout << "\nTruth table for clause '" << clause.name << "':\n";
    for (int id : asset_ids) {
        std::cout << "asset_" << id << "\t";
    }
    std::cout << "| satisfied\n";
    // Enumerate all 2^n assignments
    for (int assignment = 0; assignment < (1 << n); ++assignment) {
        std::vector<int> signed_assignment;
        for (int i = 0; i < n; ++i) {
            int id = asset_ids[i];
            bool value = (assignment & (1 << i));
            signed_assignment.push_back(value ? id : -id);
            std::cout << (value ? "+" : "-") << id << "\t";
        }
        // Check if this assignment satisfies the clause
        bool satisfied = false;
        // Clause is satisfied if any positive literal is true or any negative literal is false
        for (int pos : clause.positive_literals) {
            if (std::find(signed_assignment.begin(), signed_assignment.end(), pos) != signed_assignment.end()) {
                satisfied = true;
                break;
            }
        }
        for (int neg : clause.negative_literals) {
            if (std::find(signed_assignment.begin(), signed_assignment.end(), -neg) != signed_assignment.end()) {
                satisfied = true;
                break;
            }
        }
        std::cout << "| " << (satisfied ? "1" : "0") << "\n";
    }
}

} // namespace witness 