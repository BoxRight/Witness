#include "semantic_analyzer.hpp"
#include "conflict_analyzer.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <set>
#include <ast.hpp>
#include <sstream>
#include <fstream>
#include <cstdlib>

namespace witness {

// Global counter for unique filenames
static int global_check_counter = 0;

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
    
    // Initialize solver mode
    solverMode = "exhaustive";
    
    // Initialize verbosity flags
    verbose = false;
    quiet = false;
    
    // Initialize conflict analyzer
    conflict_analyzer = std::make_unique<ConflictAnalyzer>();
}

void SemanticAnalyzer::setSolverMode(const std::string& mode) {
    solverMode = mode;
}

std::string SemanticAnalyzer::getSolverMode() const {
    return solverMode;
}

void SemanticAnalyzer::setVerbose(bool v) {
    verbose = v;
}

void SemanticAnalyzer::setQuiet(bool q) {
    quiet = q;
}

bool SemanticAnalyzer::isVerbose() const {
    return verbose;
}

bool SemanticAnalyzer::isQuiet() const {
    return quiet;
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
                                 const std::vector<int>& negative_literals, const std::string& expression, Expression* expr) {
    ClauseInfo clause;
    clause.name = clause_name;
    clause.positive_literals = positive_literals;
    clause.negative_literals = negative_literals;
    clause.expression = expression;
    clause.expr = expr;
    current_clauses.push_back(clause);
    std::string pos_str = "";
    for (int lit : positive_literals) {
        pos_str += "+" + std::to_string(lit) + " ";
    }
    std::string neg_str = "";
    for (int lit : negative_literals) {
        neg_str += "-" + std::to_string(lit) + " ";
    }
    reportWarning("Clause '" + clause_name + "' added: [" + pos_str + neg_str + "] from '" + expression + "'");
    printClauseTruthTable(current_clauses.back());
}

SemanticAnalyzer::SatisfiabilityResult SemanticAnalyzer::generateTruthTable() {
    if (solverMode == "external") {
        generateExternalSolverTruthTable();
        
        // Read results from CUDA solver using the most recent result file
        std::string result_filename = "zdd_" + std::to_string(global_check_counter) + ".bin";
        std::ifstream result_file(result_filename, std::ios::binary);
        if (!result_file.is_open()) {
            SatisfiabilityResult result;
            result.satisfiable = false;
            result.error_message = "External solver mode: Could not read CUDA solver results";
            return result;
        }
        
        SatisfiabilityResult result;
        result.satisfiable = false;
        
        // Read combinations from binary file
        while (result_file.good()) {
            int size;
            result_file.read(reinterpret_cast<char*>(&size), sizeof(int));
            if (result_file.eof()) break;
            
            if (size > 0 && size <= 1000) { // Sanity check
                std::vector<int> combination(size);
                result_file.read(reinterpret_cast<char*>(combination.data()), size * sizeof(int));
                if (result_file.good()) {
                    result.assignments.push_back(combination);
                }
            }
        }
        result_file.close();
        
        result.satisfiable = !result.assignments.empty();
        if (result.satisfiable) {
            result.error_message = "External solver mode: " + std::to_string(result.assignments.size()) + " satisfying assignments found";
        } else {
            // Create reverse mapping from asset IDs to asset names
            std::unordered_map<int, std::string> id_to_asset;
            for (const auto& pair : asset_to_id) {
                id_to_asset[pair.second] = pair.first;
            }
            
            // Use conflict analyzer to find minimal conflicting set
            std::vector<std::string> conflicting_clauses = conflict_analyzer->findMinimalConflictingSet(current_clauses, id_to_asset);
            std::string conflict_report = conflict_analyzer->generateConflictReport(conflicting_clauses, id_to_asset);
            
            result.error_message = "External solver mode: No satisfying assignments found";
            result.conflicting_clauses = conflicting_clauses;
            
            // Print conflict report
            std::cout << "\n" << conflict_report << std::endl;
        }
        
        return result;
    }

    SatisfiabilityResult result;
    result.satisfiable = false;

    if (current_clauses.empty()) {
        result.satisfiable = true;
        result.assignments.push_back({}); // Empty assignment satisfies no clauses
        return result;
    }

    if (solverMode == "exhaustive") {
        // Use current exhaustive approach
        return generateExhaustiveTruthTable();
    } else {
        reportError("Unknown solver mode: " + solverMode);
        return result;
    }
}

SemanticAnalyzer::SatisfiabilityResult SemanticAnalyzer::generateExhaustiveTruthTable() {
    SatisfiabilityResult result;
    result.satisfiable = false;

    if (current_clauses.empty()) {
        result.satisfiable = true;
        result.assignments.push_back({}); // Empty assignment satisfies no clauses
        return result;
    }

    // Collect all unique asset IDs used in all clause expressions
    std::set<int> all_asset_ids;
    for (const auto& clause : current_clauses) {
        collectAssetIDs(clause.expr, all_asset_ids);
    }
    std::vector<int> asset_ids(all_asset_ids.begin(), all_asset_ids.end());
    int num_assets = asset_ids.size();

    reportWarning("Truth table generation: " + std::to_string(num_assets) + " assets, " +
                  std::to_string(current_clauses.size()) + " clauses, " +
                  std::to_string(1 << num_assets) + " combinations to check");

    // Generate all possible 2^n truth assignments
    for (int assignment = 0; assignment < (1 << num_assets); assignment++) {
        std::vector<int> current_assignment;
        std::map<int, bool> assignment_map;
        // Create signed literal assignment
        for (int i = 0; i < num_assets; i++) {
            int asset_id = asset_ids[i];
            bool value = (assignment & (1 << i));
            assignment_map[asset_id] = value;
            if (value) {
                current_assignment.push_back(asset_id);  // Positive literal
            } else {
                current_assignment.push_back(-asset_id); // Negative literal
            }
        }

        // Check if this assignment satisfies all clauses (using evalExpr on each clause.expr)
        bool satisfies_all = true;
        for (const auto& clause : current_clauses) {
            bool clause_satisfied = false;
            try {
                clause_satisfied = evalExpr(clause.expr, assignment_map);
            } catch (const std::exception& e) {
                reportError(std::string("[generateTruthTable] Evaluation error: ") + e.what());
                clause_satisfied = false;
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
        // Create reverse mapping from asset IDs to asset names
        std::unordered_map<int, std::string> id_to_asset;
        for (const auto& pair : asset_to_id) {
            id_to_asset[pair.second] = pair.first;
        }
        
        // Use conflict analyzer to find minimal conflicting set
        std::vector<std::string> conflicting_clauses = conflict_analyzer->findMinimalConflictingSet(current_clauses, id_to_asset);
        std::string conflict_report = conflict_analyzer->generateConflictReport(conflicting_clauses, id_to_asset);
        
        result.error_message = "No satisfying assignments found - clauses are unsatisfiable";
        result.conflicting_clauses = conflicting_clauses;
        
        reportError(result.error_message);
        std::cout << "\n" << conflict_report << std::endl;
    }

    return result;
}

SemanticAnalyzer::SatisfiabilityResult SemanticAnalyzer::generateSelectiveTruthTable(const std::vector<std::string>& target_assets) {
    SatisfiabilityResult result;
    result.satisfiable = false;

    if (current_clauses.empty()) {
        result.satisfiable = true;
        result.assignments.push_back({}); // Empty assignment satisfies no clauses
        return result;
    }

    // Get asset IDs for target assets
    std::set<int> target_asset_ids;
    for (const auto& asset_name : target_assets) {
        auto it = asset_to_id.find(asset_name);
        if (it != asset_to_id.end()) {
            target_asset_ids.insert(it->second);
        } else {
            reportWarning("Asset '" + asset_name + "' not found in current clauses - skipping");
        }
    }

    if (target_asset_ids.empty()) {
        result.satisfiable = true;
        result.assignments.push_back({}); // No target assets means trivially satisfiable
        return result;
    }

    // Filter clauses to only those that involve target assets
    std::vector<ClauseInfo> relevant_clauses;
    for (const auto& clause : current_clauses) {
        std::set<int> clause_assets;
        collectAssetIDs(clause.expr, clause_assets);
        
        // Check if this clause involves any target assets
        bool is_relevant = false;
        for (int target_id : target_asset_ids) {
            if (clause_assets.find(target_id) != clause_assets.end()) {
                is_relevant = true;
                break;
            }
        }
        
        if (is_relevant) {
            relevant_clauses.push_back(clause);
        }
    }

    if (relevant_clauses.empty()) {
        result.satisfiable = true;
        result.assignments.push_back({}); // No relevant clauses means trivially satisfiable
        return result;
    }

    // Collect all unique asset IDs used in relevant clause expressions
    std::set<int> all_asset_ids;
    for (const auto& clause : relevant_clauses) {
        collectAssetIDs(clause.expr, all_asset_ids);
    }
    std::vector<int> asset_ids(all_asset_ids.begin(), all_asset_ids.end());
    int num_assets = asset_ids.size();

    reportWarning("Selective truth table generation: " + std::to_string(num_assets) + " assets, " +
                  std::to_string(relevant_clauses.size()) + " relevant clauses, " +
                  std::to_string(1 << num_assets) + " combinations to check");

    // Generate all possible 2^n truth assignments
    for (int assignment = 0; assignment < (1 << num_assets); assignment++) {
        std::vector<int> current_assignment;
        std::map<int, bool> assignment_map;
        // Create signed literal assignment
        for (int i = 0; i < num_assets; i++) {
            int asset_id = asset_ids[i];
            bool value = (assignment & (1 << i));
            assignment_map[asset_id] = value;
            if (value) {
                current_assignment.push_back(asset_id);  // Positive literal
            } else {
                current_assignment.push_back(-asset_id); // Negative literal
            }
        }

        // Check if this assignment satisfies all relevant clauses
        bool satisfies_all = true;
        for (const auto& clause : relevant_clauses) {
            bool clause_satisfied = false;
            try {
                clause_satisfied = evalExpr(clause.expr, assignment_map);
            } catch (const std::exception& e) {
                reportError(std::string("[generateSelectiveTruthTable] Evaluation error: ") + e.what());
                clause_satisfied = false;
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
        reportWarning("Selective truth table generation completed: " + std::to_string(result.assignments.size()) + " satisfying assignments found");
    } else {
        // Create reverse mapping from asset IDs to asset names
        std::unordered_map<int, std::string> id_to_asset;
        for (const auto& pair : asset_to_id) {
            id_to_asset[pair.second] = pair.first;
        }
        
        // Use conflict analyzer to find minimal conflicting set
        std::vector<std::string> conflicting_clauses = conflict_analyzer->findMinimalConflictingSet(relevant_clauses, id_to_asset);
        std::string conflict_report = conflict_analyzer->generateConflictReport(conflicting_clauses, id_to_asset);
        
        result.error_message = "No satisfying assignments found for selected assets - clauses are unsatisfiable";
        result.conflicting_clauses = conflicting_clauses;
        
        reportError(result.error_message);
        std::cout << "\n" << conflict_report << std::endl;
    }

    return result;
}

SemanticAnalyzer::SatisfiabilityResult SemanticAnalyzer::generateSelectiveExternalTruthTable(const std::vector<std::string>& target_assets) {
    SatisfiabilityResult result;
    result.satisfiable = false;

    if (current_clauses.empty()) {
        result.satisfiable = true;
        result.assignments.push_back({}); // Empty assignment satisfies no clauses
        return result;
    }

    // Get asset IDs for target assets
    std::set<int> target_asset_ids;
    for (const auto& asset_name : target_assets) {
        auto it = asset_to_id.find(asset_name);
        if (it != asset_to_id.end()) {
            target_asset_ids.insert(it->second);
        } else {
            reportWarning("Asset '" + asset_name + "' not found in current clauses - skipping");
        }
    }

    if (target_asset_ids.empty()) {
        result.satisfiable = true;
        result.assignments.push_back({}); // No target assets means trivially satisfiable
        return result;
    }

    // Filter clauses to only those that involve target assets
    std::vector<ClauseInfo> relevant_clauses;
    for (const auto& clause : current_clauses) {
        std::set<int> clause_assets;
        collectAssetIDs(clause.expr, clause_assets);
        
        // Check if this clause involves any target assets
        bool is_relevant = false;
        for (int target_id : target_asset_ids) {
            if (clause_assets.find(target_id) != clause_assets.end()) {
                is_relevant = true;
                break;
            }
        }
        
        if (is_relevant) {
            relevant_clauses.push_back(clause);
        }
    }

    if (relevant_clauses.empty()) {
        result.satisfiable = true;
        result.assignments.push_back({}); // No relevant clauses means trivially satisfiable
        return result;
    }

    // Store original clauses and replace with relevant ones
    std::vector<ClauseInfo> original_clauses = current_clauses;
    current_clauses = relevant_clauses;

    // Generate unique filenames for this litis check
    global_check_counter++;
    std::string json_filename = "witness_export_" + std::to_string(global_check_counter) + ".json";
    std::string result_filename = "zdd_" + std::to_string(global_check_counter) + ".bin";

    // Export relevant clauses to JSON for CUDA solver
    std::ofstream json_file(json_filename);
    if (!json_file.is_open()) {
        reportError("Could not open JSON file for writing: " + json_filename);
        current_clauses = original_clauses; // Restore original clauses
        return result;
    }

    json_file << "{\n";
    json_file << "  \"clauses\": [\n";
    
    for (size_t i = 0; i < relevant_clauses.size(); ++i) {
        const auto& clause = relevant_clauses[i];
        json_file << "    {\n";
        json_file << "      \"name\": \"" << clause.name << "\",\n";
        json_file << "      \"expression\": \"" << clause.expression << "\",\n";
        json_file << "      \"assignments\": [\n";
        
        // Generate satisfying assignments for this clause
        std::set<int> clause_assets;
        collectAssetIDs(clause.expr, clause_assets);
        std::vector<int> asset_list(clause_assets.begin(), clause_assets.end());
        
        // Generate all possible assignments for this clause
        std::vector<std::vector<int>> satisfying_assignments;
        for (int assignment = 0; assignment < (1 << asset_list.size()); ++assignment) {
            std::map<int, bool> assignment_map;
            std::vector<int> current_assignment;
            
            for (size_t j = 0; j < asset_list.size(); ++j) {
                int asset_id = asset_list[j];
                bool value = (assignment & (1 << j));
                assignment_map[asset_id] = value;
                if (value) {
                    current_assignment.push_back(asset_id);
                } else {
                    current_assignment.push_back(-asset_id);
                }
            }
            
            // Check if this assignment satisfies the clause
            bool clause_satisfied = false;
            try {
                clause_satisfied = evalExpr(clause.expr, assignment_map);
            } catch (const std::exception& e) {
                clause_satisfied = false;
            }
            
            if (clause_satisfied) {
                satisfying_assignments.push_back(current_assignment);
            }
        }
        
        // Write satisfying assignments to JSON
        for (size_t assign_idx = 0; assign_idx < satisfying_assignments.size(); ++assign_idx) {
            const auto& current_assignment = satisfying_assignments[assign_idx];
            json_file << "        [";
            for (size_t k = 0; k < current_assignment.size(); ++k) {
                json_file << current_assignment[k];
                if (k < current_assignment.size() - 1) json_file << ", ";
            }
            json_file << "]";
            if (assign_idx < satisfying_assignments.size() - 1) json_file << ",";
            json_file << "\n";
        }
        
        json_file << "      ]\n";
        json_file << "    }";
        if (i < relevant_clauses.size() - 1) json_file << ",";
        json_file << "\n";
    }
    
    json_file << "  ]\n";
    json_file << "}\n";
    json_file.close();

    // Call CUDA solver
    std::string cuda_command = "./tree_fold_cuda " + json_filename + " " + result_filename;
    int exit_code = system(cuda_command.c_str());
    
    if (exit_code != 0) {
        reportError("CUDA solver failed with exit code: " + std::to_string(exit_code));
        current_clauses = original_clauses; // Restore original clauses
        return result;
    }

    // Read results from CUDA solver
    std::ifstream result_file(result_filename, std::ios::binary);
    if (!result_file.is_open()) {
        reportError("Could not open result file: " + result_filename);
        current_clauses = original_clauses; // Restore original clauses
        return result;
    }

    // Read combinations from binary file
    while (result_file.good()) {
        int size;
        result_file.read(reinterpret_cast<char*>(&size), sizeof(int));
        if (result_file.eof()) break;
        
        if (size > 0 && size <= 1000) { // Sanity check
            std::vector<int> combination(size);
            result_file.read(reinterpret_cast<char*>(combination.data()), size * sizeof(int));
            if (result_file.good()) {
                result.assignments.push_back(combination);
            }
        }
    }
    result_file.close();

    // Restore original clauses
    current_clauses = original_clauses;

    result.satisfiable = !result.assignments.empty();
    
    if (result.satisfiable) {
        result.error_message = "External solver mode: " + std::to_string(result.assignments.size()) + " satisfying assignments found for selected assets";
    } else {
        // Create reverse mapping from asset IDs to asset names
        std::unordered_map<int, std::string> id_to_asset;
        for (const auto& pair : asset_to_id) {
            id_to_asset[pair.second] = pair.first;
        }
        
        // Use conflict analyzer to find minimal conflicting set
        std::vector<std::string> conflicting_clauses = conflict_analyzer->findMinimalConflictingSet(relevant_clauses, id_to_asset);
        std::string conflict_report = conflict_analyzer->generateConflictReport(conflicting_clauses, id_to_asset);
        
        result.error_message = "External solver mode: No satisfying assignments found for selected assets";
        result.conflicting_clauses = conflicting_clauses;
        
        reportError(result.error_message);
        std::cout << "\n" << conflict_report << std::endl;
    }

    return result;
}

SemanticAnalyzer::SatisfiabilityResult SemanticAnalyzer::generateMeetAnalysis(const std::string& left_asset, const std::string& right_asset) {
    SatisfiabilityResult result;
    result.satisfiable = false;

    // Get the asset components for both assets
    TypeInfo* left_info = lookupType(left_asset);
    TypeInfo* right_info = lookupType(right_asset);
    
    if (!left_info || left_info->type_keyword != "asset") {
        result.error_message = "Asset '" + left_asset + "' not found or not a valid asset";
        return result;
    }
    
    if (!right_info || right_info->type_keyword != "asset") {
        result.error_message = "Asset '" + right_asset + "' not found or not a valid asset";
        return result;
    }
    
    const std::vector<std::string>& left_components = left_info->asset_components;
    const std::vector<std::string>& right_components = right_info->asset_components;
    
    if (left_components.size() < 3 || right_components.size() < 3) {
        result.error_message = "Assets must have at least 3 components (subject, action, object)";
        return result;
    }
    
    // Extract common elements between the two assets
    std::vector<std::string> common_elements;
    
    // Check for common subjects
    if (left_components[0] == right_components[0]) {
        common_elements.push_back("subject: " + left_components[0]);
    }
    
    // Check for common objects
    if (left_components[2] == right_components[2]) {
        common_elements.push_back("object: " + left_components[2]);
    }
    
    // Check for common actions (or similar actions)
    if (left_components[1] == right_components[1]) {
        common_elements.push_back("action: " + left_components[1]);
    }
    
    // Check for cross-subject/object relationships
    if (left_components[0] == right_components[2]) {
        common_elements.push_back("subject-object: " + left_components[0] + " ↔ " + right_components[2]);
    }
    
    if (left_components[2] == right_components[0]) {
        common_elements.push_back("object-subject: " + left_components[2] + " ↔ " + right_components[0]);
    }
    
    // Create a result representing the common elements
    if (!common_elements.empty()) {
        result.satisfiable = true;
        result.error_message = "Meet analysis: Found " + std::to_string(common_elements.size()) + " common elements";
        
        // Create a symbolic assignment representing the common elements
        std::vector<int> common_assignment;
        for (const auto& element : common_elements) {
            // Use a hash of the element as a symbolic ID
            std::hash<std::string> hasher;
            int element_id = static_cast<int>(hasher(element)) % 1000; // Keep it reasonable
            common_assignment.push_back(element_id);
        }
        result.assignments.push_back(common_assignment);
        
        // Report the common elements
        reportWarning("Common elements between '" + left_asset + "' and '" + right_asset + "':");
        for (const auto& element : common_elements) {
            reportWarning("  - " + element);
        }
        
        // Store the common components for asset creation
        result.common_components = common_elements;
    } else {
        result.error_message = "Meet analysis: No common elements found between '" + left_asset + "' and '" + right_asset + "'";
        reportWarning("No common elements found between assets:");
        reportWarning("  Left:  (" + left_components[0] + ", " + left_components[1] + ", " + left_components[2] + ")");
        reportWarning("  Right: (" + right_components[0] + ", " + right_components[1] + ", " + right_components[2] + ")");
    }

    return result;
}

void SemanticAnalyzer::processDeferredMeetOperations() {
    if (deferred_meet_operations.empty()) {
        return;
    }
    
    reportWarning("Processing " + std::to_string(deferred_meet_operations.size()) + " deferred meet() operations...");
    
    for (const auto& deferred_op : deferred_meet_operations) {
        reportWarning("Processing deferred meet() operation: " + deferred_op.left_asset + " and " + deferred_op.right_asset);
        
        // Perform meet operation analysis
        SatisfiabilityResult result = generateMeetAnalysis(deferred_op.left_asset, deferred_op.right_asset);
        
        if (result.satisfiable) {
            reportWarning("Deferred meet() operation successful - common legal ground found");
            
            // Report common elements
            for (size_t i = 0; i < result.assignments.size(); i++) {
                std::string assignment_str = "Common assignment " + std::to_string(i + 1) + ": [";
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
            std::cout << "Meet check SATISFIABLE" << std::endl;
        } else {
            reportError("Deferred meet() operation failed - no common legal ground found: " + result.error_message);
            std::cout << "Meet check UNSATISFIABLE: " << result.error_message << std::endl;
        }
        
        // Reset clause set for next operation
        current_clauses.clear();
        reportWarning("Clause set reset after deferred meet() operation.");
    }
    
    // Clear the deferred operations after processing
    deferred_meet_operations.clear();
}

void SemanticAnalyzer::exportForCudaSolver(const std::vector<std::set<std::vector<int>>>& clause_satisfying_assignments, 
                                           const std::set<int>& all_asset_ids) {
    std::cout << "\n=== CUDA SOLVER EXPORT (CudaSet Format) ===" << std::endl;
    
    // Prepare data for CudaSet structure - TRULY FLATTENED
    std::vector<int8_t> flattened_data;
    std::vector<int> offsets;
    std::vector<int> sizes;
    
    int current_offset = 0;
    std::vector<int> asset_list(all_asset_ids.begin(), all_asset_ids.end());
    int assets_per_assignment = asset_list.size();
    
    for (size_t clause_idx = 0; clause_idx < clause_satisfying_assignments.size(); clause_idx++) {
        const auto& assignments = clause_satisfying_assignments[clause_idx];
        
        offsets.push_back(current_offset);
        sizes.push_back(assignments.size());
        
        // Flatten this clause's assignments into the data array
        // Each assignment is a fixed-size array of asset values
        for (const auto& assignment : assignments) {
            // Create a complete assignment vector for all assets (not just clause assets)
            std::vector<int8_t> complete_assignment(assets_per_assignment, 0);
            
            // Fill in the values for assets that are in this clause
            for (size_t i = 0; i < assignment.size(); i++) {
                int asset_id = std::abs(assignment[i]);
                int sign = assignment[i] > 0 ? 1 : -1;
                
                // Find position of this asset in the global asset list
                auto it = std::find(asset_list.begin(), asset_list.end(), asset_id);
                if (it != asset_list.end()) {
                    int pos = std::distance(asset_list.begin(), it);
                    complete_assignment[pos] = static_cast<int8_t>(sign * asset_id);
                }
            }
            
            // Add the complete assignment to flattened data
            flattened_data.insert(flattened_data.end(), complete_assignment.begin(), complete_assignment.end());
        }
        
        current_offset = flattened_data.size();
    }
    
    // Output in CudaSet-compatible format
    std::cout << "# CudaSet Format - Copy this data to your CUDA program" << std::endl;
    std::cout << "# Format: numItems totalElements" << std::endl;
    std::cout << "# Then: offset1 offset2 ... offsetN" << std::endl;
    std::cout << "# Then: size1 size2 ... sizeN" << std::endl;
    std::cout << "# Then: data1 data2 ... dataM" << std::endl;
    std::cout << std::endl;
    
    std::cout << "# Header" << std::endl;
    std::cout << clause_satisfying_assignments.size() << " " << flattened_data.size() << std::endl;
    
    std::cout << "# Offsets" << std::endl;
    for (int offset : offsets) {
        std::cout << offset << " ";
    }
    std::cout << std::endl;
    
    std::cout << "# Sizes" << std::endl;
    for (int size : sizes) {
        std::cout << size << " ";
    }
    std::cout << std::endl;
    
    std::cout << "# Flattened Data (int8_t values)" << std::endl;
    for (size_t i = 0; i < flattened_data.size(); i++) {
        std::cout << static_cast<int>(flattened_data[i]);
        if (i < flattened_data.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    
    // Also output asset mapping for reference
    std::cout << std::endl;
    std::cout << "# Asset ID Mapping (for reference)" << std::endl;
    for (size_t i = 0; i < asset_list.size(); i++) {
        std::cout << "Asset " << asset_list[i] << " -> Position " << i << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "# CudaSet Structure Summary:" << std::endl;
    std::cout << "# - numItems: " << clause_satisfying_assignments.size() << std::endl;
    std::cout << "# - totalElements: " << flattened_data.size() << std::endl;
    std::cout << "# - Each clause has 'size' assignments" << std::endl;
    std::cout << "# - Each assignment has " << assets_per_assignment << " values (one per global asset)" << std::endl;
    std::cout << "# - Positive values = asset is true, negative = asset is false" << std::endl;
    std::cout << "# - Zero values = asset not involved in this clause" << std::endl;
    std::cout << "=== END CUDA SOLVER EXPORT ===" << std::endl;
}

bool SemanticAnalyzer::assignmentsCompatible(const std::vector<int>& assignment1, const std::vector<int>& assignment2) {
    // Check if two assignments are compatible (no conflicting values for same asset)
    std::map<int, bool> values1, values2;
    
    for (int lit : assignment1) {
        int asset_id = abs(lit);
        bool value = (lit > 0);
        values1[asset_id] = value;
    }
    
    for (int lit : assignment2) {
        int asset_id = abs(lit);
        bool value = (lit > 0);
        
        if (values1.find(asset_id) != values1.end()) {
            if (values1[asset_id] != value) {
                return false; // Conflict found
            }
        }
    }
    
    return true;
}

std::vector<int> SemanticAnalyzer::mergeAssignments(const std::vector<int>& assignment1, const std::vector<int>& assignment2) {
    // Merge two compatible assignments
    std::map<int, bool> merged_values;
    
    for (int lit : assignment1) {
        int asset_id = abs(lit);
        bool value = (lit > 0);
        merged_values[asset_id] = value;
    }
    
    for (int lit : assignment2) {
        int asset_id = abs(lit);
        bool value = (lit > 0);
        merged_values[asset_id] = value;
    }
    
    std::vector<int> result;
    for (const auto& pair : merged_values) {
        if (pair.second) {
            result.push_back(pair.first);
        } else {
            result.push_back(-pair.first);
        }
    }
    
    return result;
}

void SemanticAnalyzer::analyze(Program* program) {
    if (!program) {
        reportError("Cannot analyze null program");
        return;
    }
    
    // Clean up previous JSON files to prevent CUDA program from processing stale data
    if (verbose) {
        std::cout << "Cleaning up previous JSON files..." << std::endl;
    }
    
    // Remove all witness_export_*.json files from previous runs
    std::string cleanup_command = "rm -f witness_export_*.json";
    int cleanup_result = system(cleanup_command.c_str());
    if (cleanup_result != 0 && verbose) {
        std::cout << "Warning: Could not clean up all previous JSON files" << std::endl;
    }
    
    // Remove all zdd_*.bin files from previous runs
    cleanup_command = "rm -f zdd_*.bin";
    cleanup_result = system(cleanup_command.c_str());
    if (cleanup_result != 0 && verbose) {
        std::cout << "Warning: Could not clean up all previous ZDD files" << std::endl;
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
    if (!errors.empty() && !quiet) {
        std::cout << "Semantic Analysis Errors:" << std::endl;
        for (const auto& error : errors) {
            std::cout << "  Error: " << error << std::endl;
        }
    }
    
    if (!warnings.empty() && !quiet) {
        std::cout << "Semantic Analysis Warnings:" << std::endl;
        for (const auto& warning : warnings) {
            std::cout << "  Warning: " << warning << std::endl;
        }
    }
    

    
    // Report completion status
    if (!quiet) {
        if (errors.empty()) {
            std::cout << "Semantic analysis completed successfully!" << std::endl;
            std::cout << "- System operations validated: global(), domain(), litis(), meet()" << std::endl;
            std::cout << "- Join operations validated: transfer, sell, compensation, consideration, forbearance, encumber" << std::endl;
            std::cout << "- Logical operations validated: oblig(), claim(), not()" << std::endl;
        } else {
            std::cout << "Semantic analysis completed with " << errors.size() << " error(s)" << std::endl;
        }
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
    
    // Check if this is a join operation (single function call)
    if (asset_def->value->expressions.size() == 1) {
        auto& expr = asset_def->value->expressions[0];
        
        if (auto func_call = dynamic_cast<FunctionCallExpression*>(expr.get())) {
            if (func_call->function_name && isJoinOperation(func_call->function_name->name)) {
                // This is a join operation - compute combined components
                std::string join_type = func_call->function_name->name;
                
                if (func_call->arguments && func_call->arguments->expressions.size() == 2) {
                    auto left_arg = func_call->arguments->expressions[0].get();
                    auto right_arg = func_call->arguments->expressions[1].get();
                    
                    // Get components from left and right assets
                    auto left_components = getAssetComponents(left_arg);
                    auto right_components = getAssetComponents(right_arg);
                    
                    // Debug: Report component sizes
                    reportWarning("Join validation for '" + name + "': left_components.size()=" + 
                                 std::to_string(left_components.size()) + ", right_components.size()=" + 
                                 std::to_string(right_components.size()));
                    
                    if (left_components.size() >= 3 && right_components.size() >= 3) {
                        // Create combined components based on join type
                        if (join_type == "join") {
                            // Universal join: combine subject and object, merge actions
                            components.push_back(left_components[0]); // left subject
                            components.push_back(left_components[1] + "_" + right_components[1]); // combined action
                            components.push_back(left_components[2]); // left object
                        } else {
                            // For other join types, use the same pattern for now
                            components.push_back(left_components[0]); // left subject
                            components.push_back(join_type + "_" + left_components[1] + "_" + right_components[1]); // join_type + combined action
                            components.push_back(left_components[2]); // left object
                        }
                        
                        reportWarning("Join asset '" + name + "' created with components: (" + 
                                     components[0] + ", " + components[1] + ", " + components[2] + ")");
                        
                        // Debug: Check if components are valid
                        if (components[0].empty() || components[1].empty() || components[2].empty()) {
                            reportError("Join asset '" + name + "' has empty components - this will cause issues");
                        }
                    } else {
                        reportError("Join operation requires assets with at least 3 components each");
                        return;
                    }
                } else {
                    reportError("Join operation requires exactly 2 arguments");
                    return;
                }
            } else if (func_call->function_name && isSystemOperation(func_call->function_name->name)) {
                // This is a system operation
                if (func_call->function_name->name == "meet") {
                    // Handle meet operation - it will create the asset itself
                    validateMeetOperation(func_call, name);
                    return; // The meet operation handles asset creation
                } else {
                    // Other system operations, analyze normally
                    analyzeExpression(expr.get());
                    return;
                }
            } else {
                // Not a join or system operation, analyze normally
                analyzeExpression(expr.get());
                return;
            }
        } else {
            // Not a function call, analyze normally
            analyzeExpression(expr.get());
            return;
        }
    } else {
        // Extract asset components from the expression list (basic asset definition)
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
        // Robust type checking for asset construction (basic asset definition only)
        if (components.size() != 3) {
            reportError("Asset '" + name + "' must have exactly 3 components (subject/authority, service/action/time, subject/authority)");
            throw std::runtime_error("Asset construction error");
        }
        auto check_type = [&](const std::string& sym, std::initializer_list<std::string> allowed_types) -> bool {
            auto it = symbol_table.find(sym);
            if (it == symbol_table.end()) return false;
            for (const auto& t : allowed_types) {
                if (it->second.type_keyword == t) return true;
            }
            return false;
        };
        // 1st component: subject/authority
        if (!check_type(components[0], {"subject", "authority"})) {
            reportError("First component of asset '" + name + "' must be a defined subject or authority (got '" + components[0] + "')");
            throw std::runtime_error("Asset construction error");
        }
        // 2nd component: service/action/time
        if (!check_type(components[1], {"service", "action", "time"})) {
            reportError("Second component of asset '" + name + "' must be a defined service, action, or time (got '" + components[1] + "')");
            throw std::runtime_error("Asset construction error");
        }
        // 3rd component: subject/authority
        if (!check_type(components[2], {"subject", "authority"})) {
            reportError("Third component of asset '" + name + "' must be a defined subject or authority (got '" + components[2] + "')");
            throw std::runtime_error("Asset construction error");
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
    
    // Check for associativity in complex join expressions
    if (!validateJoinAssociativity(join_type, left_asset, right_asset)) {
        return false;
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

bool SemanticAnalyzer::validateJoinAssociativity(const std::string& join_type, 
                                                Expression* left_asset, 
                                                Expression* right_asset) {
    // Check if either argument is itself a join operation of the same type
    // This indicates a nested join that needs associativity validation
    
    bool left_is_join = false;
    bool right_is_join = false;
    std::string left_join_type = "";
    std::string right_join_type = "";
    
    // Check if left argument is a join operation
    if (auto left_func_call = dynamic_cast<FunctionCallExpression*>(left_asset)) {
        if (left_func_call->function_name && isJoinOperation(left_func_call->function_name->name)) {
            left_is_join = true;
            left_join_type = left_func_call->function_name->name;
        }
    }
    
    // Check if right argument is a join operation
    if (auto right_func_call = dynamic_cast<FunctionCallExpression*>(right_asset)) {
        if (right_func_call->function_name && isJoinOperation(right_func_call->function_name->name)) {
            right_is_join = true;
            right_join_type = right_func_call->function_name->name;
        }
    }
    
    // If both arguments are joins of the same type, validate associativity
    if (left_is_join && right_is_join && left_join_type == right_join_type && left_join_type == join_type) {
        // This is a case like: join(join(a,b), join(c,d))
        // We need to validate that the structure allows for associativity
        
        // For now, we'll report this as a complex nested join that may need manual validation
        reportWarning("Complex nested " + join_type + " operation detected: " + 
                     join_type + "(" + join_type + "(...), " + join_type + "(...)) - " +
                     "Associativity validation may require manual review");
        return true; // Allow it for now, but warn the user
    }
    
    // If only one argument is a join of the same type, this is the associativity case we want to validate
    if ((left_is_join && left_join_type == join_type) || 
        (right_is_join && right_join_type == join_type)) {
        
        // Extract the components for associativity validation
        std::vector<std::string> all_components;
        
        if (left_is_join && left_join_type == join_type) {
            // Case: join(join(a,b), c) - we need to validate this equals join(a, join(b,c))
            auto left_func_call = dynamic_cast<FunctionCallExpression*>(left_asset);
            if (left_func_call && left_func_call->arguments && left_func_call->arguments->expressions.size() == 2) {
                auto left_left = left_func_call->arguments->expressions[0].get();
                auto left_right = left_func_call->arguments->expressions[1].get();
                
                // Get components for associativity check
                auto left_left_components = getAssetComponents(left_left);
                auto left_right_components = getAssetComponents(left_right);
                auto right_components = getAssetComponents(right_asset);
                
                // For associativity, we need to check if the join type supports it
                // Most join operations are associative, but some contextual joins may not be
                if (join_type == "join" || join_type == "evidence" || join_type == "argument") {
                    // Universal joins are always associative
                    reportWarning("Associative " + join_type + " operation validated: " + 
                                join_type + "(" + join_type + "(a,b), c) = " + join_type + "(a, " + join_type + "(b,c))");
                    return true;
                } else {
                    // Contextual joins may have associativity constraints
                    // For now, we'll validate that the components are compatible
                    bool associativity_valid = validateContextualJoinAssociativity(join_type, 
                                                                                  left_left_components, 
                                                                                  left_right_components, 
                                                                                  right_components);
                    if (associativity_valid) {
                        reportWarning("Associative " + join_type + " operation validated");
                    } else {
                        reportError("Non-associative " + join_type + " operation: " + 
                                  join_type + "(" + join_type + "(a,b), c) ≠ " + join_type + "(a, " + join_type + "(b,c))");
                    }
                    return associativity_valid;
                }
            }
        } else if (right_is_join && right_join_type == join_type) {
            // Case: join(a, join(b,c)) - this is the right-associative form
            auto right_func_call = dynamic_cast<FunctionCallExpression*>(right_asset);
            if (right_func_call && right_func_call->arguments && right_func_call->arguments->expressions.size() == 2) {
                auto right_left = right_func_call->arguments->expressions[0].get();
                auto right_right = right_func_call->arguments->expressions[1].get();
                
                // Get components for associativity check
                auto left_components = getAssetComponents(left_asset);
                auto right_left_components = getAssetComponents(right_left);
                auto right_right_components = getAssetComponents(right_right);
                
                // Similar validation as above
                if (join_type == "join" || join_type == "evidence" || join_type == "argument") {
                    reportWarning("Associative " + join_type + " operation validated: " + 
                                join_type + "(a, " + join_type + "(b,c)) = " + join_type + "(" + join_type + "(a,b), c)");
                    return true;
                } else {
                    bool associativity_valid = validateContextualJoinAssociativity(join_type, 
                                                                                  left_components, 
                                                                                  right_left_components, 
                                                                                  right_right_components);
                    if (associativity_valid) {
                        reportWarning("Associative " + join_type + " operation validated");
                    } else {
                        reportError("Non-associative " + join_type + " operation: " + 
                                  join_type + "(a, " + join_type + "(b,c)) ≠ " + join_type + "(" + join_type + "(a,b), c)");
                    }
                    return associativity_valid;
                }
            }
        }
    }
    
    // No nested joins detected, associativity validation not needed
    return true;
}

bool SemanticAnalyzer::validateContextualJoinAssociativity(const std::string& join_type,
                                                          const std::vector<std::string>& a_components,
                                                          const std::vector<std::string>& b_components,
                                                          const std::vector<std::string>& c_components) {
    // For contextual joins, associativity depends on the specific join type and component constraints
    // This is a simplified validation - in practice, you might need more sophisticated logic
    
    if (join_type == "transfer") {
        // transfer is associative if all components are movable objects
        return true; // Simplified - assume transfer is associative
    } else if (join_type == "compensation") {
        // compensation is associative if all components are positive services
        return true; // Simplified - assume compensation is associative
    } else if (join_type == "consideration") {
        // consideration is associative if the pattern of positive/negative services is maintained
        return true; // Simplified - assume consideration is associative
    } else if (join_type == "forbearance") {
        // forbearance is associative if all components are negative services
        return true; // Simplified - assume forbearance is associative
    } else if (join_type == "encumber" || join_type == "access" || join_type == "lien") {
        // These are associative if the object/service pattern is maintained
        return true; // Simplified - assume these are associative
    }
    
    // Default: assume associative unless proven otherwise
    return true;
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
        return validateMeetOperation(func_call, "");
    } else if (operation_type == "domain") {
        return validateDomainOperation(func_call);
    }
    
    reportError("Unknown system operation: " + operation_type);
    return false;
}

void SemanticAnalyzer::reportError(const std::string& message) {
    errors.push_back(message);
    // Always print errors, even in quiet mode
    std::cerr << "Error: " << message << std::endl;
}

void SemanticAnalyzer::reportWarning(const std::string& message) {
    warnings.push_back(message);
    // Don't print warnings immediately in quiet mode, but still collect them
    // They will be shown in the summary if quiet mode is disabled
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
                        addClause(clause_name, {asset_id}, {}, function_name + "(" + identifier->name + ")", expr);
                    }
                }
            } else if (function_name == "not") {
                // not() creates negative literals
                if (func_call->arguments && func_call->arguments->expressions.size() == 1) {
                    auto arg = func_call->arguments->expressions[0].get();
                    if (auto identifier = dynamic_cast<Identifier*>(arg)) {
                        // not(asset_name) - simple case
                        int asset_id = getOrAssignAssetID(identifier->name);
                        addClause(clause_name, {}, {asset_id}, "not(" + identifier->name + ")", expr);
                    } else if (auto nested_func = dynamic_cast<FunctionCallExpression*>(arg)) {
                        // not(function_call) - nested case like not(oblig(asset))
                        std::string nested_func_name = nested_func->function_name->name;
                        if ((nested_func_name == "oblig" || nested_func_name == "claim") && 
                            nested_func->arguments && nested_func->arguments->expressions.size() == 1) {
                            auto nested_arg = nested_func->arguments->expressions[0].get();
                            if (auto nested_identifier = dynamic_cast<Identifier*>(nested_arg)) {
                                int asset_id = getOrAssignAssetID(nested_identifier->name);
                                addClause(clause_name, {}, {asset_id}, "not(" + nested_func_name + "(" + nested_identifier->name + "))", expr);
                            }
                        }
                    }
                }
            }
            
            // Still validate the operation
            validateLogicalOperation(function_name, func_call);
        } else {
            // Not a logical operation, analyze normally
            analyzeExpression(expr);
        }
    } else if (auto bin = dynamic_cast<BinaryOpExpression*>(expr)) {
        // For binary logical operations, register the clause with the full expression
        addClause(clause_name, {}, {}, "binary_op", expr);
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
        std::cout << "Global check SATISFIABLE" << std::endl;
    } else {
        reportError("global() operation failed - system is unsatisfiable: " + result.error_message);
        std::cout << "Global check UNSATISFIABLE: " << result.error_message << std::endl;
        return false;
    }
    
    // Reset clause set for next global block
    current_clauses.clear();
    reportWarning("Clause set reset after global() operation.");
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
    
    // Extract asset names from arguments
    std::vector<std::string> target_assets;
    for (const auto& arg : func_call->arguments->expressions) {
        if (!arg) {
            reportError("litis() operation requires valid asset arguments");
            return false;
        }
        
        // Extract asset name from identifier
        if (auto identifier = dynamic_cast<Identifier*>(arg.get())) {
            target_assets.push_back(identifier->name);
        } else {
            reportError("litis() operation requires asset identifier arguments");
            return false;
        }
    }
    
    // Trigger selective satisfiability checking for specified assets
    reportWarning("litis() operation triggered - selective satisfiability checking for assets: " + 
                  [&target_assets]() {
                      std::string result;
                      for (size_t i = 0; i < target_assets.size(); ++i) {
                          if (i > 0) result += ", ";
                          result += target_assets[i];
                      }
                      return result;
                  }());
    
    // Perform selective satisfiability checking based on solver mode
    SatisfiabilityResult result;
    if (solverMode == "external") {
        result = generateSelectiveExternalTruthTable(target_assets);
    } else {
        result = generateSelectiveTruthTable(target_assets);
    }
    
    if (result.satisfiable) {
        reportWarning("litis() operation successful - selected assets are satisfiable together");
        
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
        std::cout << "Litis check SATISFIABLE" << std::endl;
    } else {
        reportError("litis() operation failed - selected assets are unsatisfiable: " + result.error_message);
        std::cout << "Litis check UNSATISFIABLE: " << result.error_message << std::endl;
        return false;
    }
    
    // Reset clause set for next operation
    current_clauses.clear();
    reportWarning("Clause set reset after litis() operation.");
    return true;
}

bool SemanticAnalyzer::validateMeetOperation(FunctionCallExpression* func_call, const std::string& asset_name) {
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
    
    // Extract asset names from arguments
    std::string left_asset_name, right_asset_name;
    
    if (auto left_identifier = dynamic_cast<Identifier*>(left_arg)) {
        left_asset_name = left_identifier->name;
    } else {
        reportError("meet() operation requires asset identifier arguments");
        return false;
    }
    
    if (auto right_identifier = dynamic_cast<Identifier*>(right_arg)) {
        right_asset_name = right_identifier->name;
    } else {
        reportError("meet() operation requires asset identifier arguments");
        return false;
    }
    
    // Perform meet operation analysis for greatest common legal denominator extraction
    reportWarning("meet() operation triggered - extracting greatest common legal denominator from: " + 
                  left_asset_name + " and " + right_asset_name);
    
    // Perform meet operation analysis
    SatisfiabilityResult result = generateMeetAnalysis(left_asset_name, right_asset_name);
    
    if (result.satisfiable) {
        reportWarning("meet() operation successful - common elements found");
        
        // Create a new asset from the common components
        if (!asset_name.empty() && !result.common_components.empty()) {
            // Create a simplified asset representation from common elements
            std::vector<std::string> asset_components;
            
            // Extract common subject, action, and object from common elements
            std::string common_subject = "";
            std::string common_action = "meet";
            std::string common_object = "";
            
            for (const auto& element : result.common_components) {
                if (element.find("subject: ") == 0) {
                    common_subject = element.substr(9); // Remove "subject: " prefix
                } else if (element.find("object: ") == 0) {
                    common_object = element.substr(8); // Remove "object: " prefix
                } else if (element.find("subject-object: ") == 0) {
                    // Handle cross-relationship
                    std::string relationship = element.substr(15); // Remove "subject-object: " prefix
                    size_t arrow_pos = relationship.find(" ↔ ");
                    if (arrow_pos != std::string::npos) {
                        common_subject = relationship.substr(0, arrow_pos);
                        common_object = relationship.substr(arrow_pos + 3);
                    }
                }
            }
            
            // Always create an asset with exactly 3 components
            asset_components.push_back(common_subject.empty() ? "shared" : common_subject);
            asset_components.push_back(common_action);
            asset_components.push_back(common_object.empty() ? "shared" : common_object);
            
            // Register the new meet asset in the symbol table
            symbol_table[asset_name] = TypeInfo("asset", "", asset_components);
            
            reportWarning("Created meet asset '" + asset_name + "' with components: (" + 
                         asset_components[0] + ", " + asset_components[1] + ", " + asset_components[2] + ")");
        }
        
        std::cout << "Meet check SATISFIABLE" << std::endl;
    } else {
        reportError("meet() operation failed - no common elements found: " + result.error_message);
        std::cout << "Meet check UNSATISFIABLE: " << result.error_message << std::endl;
        return false;
    }
    
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

void SemanticAnalyzer::printClauseTruthTable(const witness::ClauseInfo& clause) {
    if (!verbose) return; // Only print if verbose mode is enabled
    
    if (!clause.expr) {
        std::cerr << "[printClauseTruthTable] Error: No expression pointer for clause '" << clause.name << "'.\n";
        return;
    }
    std::set<int> asset_id_set;
    collectAssetIDs(clause.expr, asset_id_set);
    std::vector<int> asset_ids(asset_id_set.begin(), asset_id_set.end());
    int n = asset_ids.size();
    if (n == 0) {
        std::cout << "Clause '" << clause.name << "' has no asset variables.\n";
        return;
    }
    std::cout << "\nTruth table for clause '" << clause.name << "':\n";
    for (int id : asset_ids) {
        std::cout << "asset_" << id << "\t";
    }
    std::cout << "| satisfied\n";
    for (int assignment = 0; assignment < (1 << n); ++assignment) {
        std::map<int, bool> assignment_map;
        for (int i = 0; i < n; ++i) {
            int id = asset_ids[i];
            bool value = (assignment & (1 << i));
            assignment_map[id] = value;
            std::cout << (value ? "+" : "-") << id << "\t";
        }
        bool satisfied = false;
        try {
            satisfied = evalExpr(clause.expr, assignment_map);
        } catch (const std::exception& e) {
            std::cerr << "[printClauseTruthTable] Evaluation error: " << e.what() << "\n";
        }
        std::cout << "| " << (satisfied ? "1" : "0") << "\n";
    }
}



void SemanticAnalyzer::collectAssetIDs(Expression* expr, std::set<int>& ids) {
    if (!expr) return;

    if (auto identifier = dynamic_cast<Identifier*>(expr)) {
        // Assign asset ID if not already assigned
        int asset_id = getOrAssignAssetID(identifier->name);
        ids.insert(asset_id);
    }
    else if (auto func_call = dynamic_cast<FunctionCallExpression*>(expr)) {
        if (func_call->arguments) {
            for (const auto& arg : func_call->arguments->expressions) {
                collectAssetIDs(arg.get(), ids);
            }
        }
    }
    else if (auto binary_op = dynamic_cast<BinaryOpExpression*>(expr)) {
        collectAssetIDs(binary_op->left.get(), ids);
        collectAssetIDs(binary_op->right.get(), ids);
    }
    else if (auto unary_op = dynamic_cast<UnaryOpExpression*>(expr)) {
        collectAssetIDs(unary_op->operand.get(), ids);
    }
    // Add other expression types as needed (ExpressionList, etc.)
}

bool SemanticAnalyzer::evalExpr(Expression* expr, const std::map<int, bool>& assignment) {
    if (!expr) return false;
    
    if (auto identifier = dynamic_cast<Identifier*>(expr)) {
        // Look up asset ID and return its value from assignment
        auto it = asset_to_id.find(identifier->name);
        if (it != asset_to_id.end()) {
            int asset_id = it->second;
            auto assignment_it = assignment.find(asset_id);
            if (assignment_it != assignment.end()) {
                return assignment_it->second;
            }
        }
        return false; // Asset not found in assignment
    }
    else if (auto func_call = dynamic_cast<FunctionCallExpression*>(expr)) {
        std::string function_name = func_call->function_name->name;
        
        if (function_name == "oblig" || function_name == "claim") {
            // oblig(asset) and claim(asset) return the asset's value
            if (func_call->arguments && func_call->arguments->expressions.size() == 1) {
                return evalExpr(func_call->arguments->expressions[0].get(), assignment);
            }
        }
        else if (function_name == "not") {
            // not(expr) returns the negation of expr
            if (func_call->arguments && func_call->arguments->expressions.size() == 1) {
                return !evalExpr(func_call->arguments->expressions[0].get(), assignment);
            }
        }
        return false;
    }
    else if (auto binary_op = dynamic_cast<BinaryOpExpression*>(expr)) {
        bool left_val = evalExpr(binary_op->left.get(), assignment);
        bool right_val = evalExpr(binary_op->right.get(), assignment);
        
        // Handle different binary operators based on your TokenType enum
        if (binary_op->op == "IMPLIES") {
            return !left_val || right_val;  // A ⇒ B ≡ ¬A ∨ B
        }
        else if (binary_op->op == "AND") {
            return left_val && right_val;
        }
        else if (binary_op->op == "OR") {
            return left_val || right_val;
        }
        else if (binary_op->op == "XOR") {
            return left_val != right_val;   // A ⊕ B ≡ (A ∧ ¬B) ∨ (¬A ∧ B)
        }
        else if (binary_op->op == "EQUIV") {
            return left_val == right_val;   // A ⇔ B ≡ (A ∧ B) ∨ (¬A ∧ ¬B)
        }
        
        return false; // Unknown operator
    }
    else if (auto unary_op = dynamic_cast<UnaryOpExpression*>(expr)) {
        bool operand_val = evalExpr(unary_op->operand.get(), assignment);
        
        // Handle unary operators
        if (unary_op->op == "not") {
            return !operand_val;
        }
        
        return false; // Unknown operator
    }
    
    return false; // Default case
}

void SemanticAnalyzer::generateExternalSolverTruthTable() {
    if (current_clauses.empty()) {
        std::cout << "No clauses to process for external solver." << std::endl;
        return;
    }

    // Collect all unique asset IDs from all clauses
    std::set<int> all_asset_ids;
    for (const auto& clause : current_clauses) {
        std::set<int> clause_asset_ids;
        collectAssetIDs(clause.expr, clause_asset_ids);
        all_asset_ids.insert(clause_asset_ids.begin(), clause_asset_ids.end());
    }
    std::vector<int> asset_list(all_asset_ids.begin(), all_asset_ids.end());
    int assets_per_assignment = asset_list.size();

    if (verbose) {
        std::cout << "\n=== EXTERNAL SOLVER DEBUG: Clause Sets ===" << std::endl;
    }
    std::vector<std::set<std::vector<int>>> clause_satisfying_assignments;
    
    for (size_t clause_idx = 0; clause_idx < current_clauses.size(); clause_idx++) {
        const auto& clause = current_clauses[clause_idx];
        std::set<int> clause_asset_ids;
        collectAssetIDs(clause.expr, clause_asset_ids);
        
        std::set<std::vector<int>> clause_assignments;
        
        // Generate all possible assignments for this clause's assets
        std::vector<int> asset_list_clause(clause_asset_ids.begin(), clause_asset_ids.end());
        int num_combinations = 1 << asset_list_clause.size();
        
        for (int i = 0; i < num_combinations; i++) {
            std::map<int, bool> assignment;
            for (size_t j = 0; j < asset_list_clause.size(); j++) {
                assignment[asset_list_clause[j]] = (i >> j) & 1;
            }
            
            if (evalExpr(clause.expr, assignment)) {
                std::vector<int> satisfying_assignment;
                for (int asset_id : asset_list_clause) {
                    satisfying_assignment.push_back(assignment[asset_id] ? asset_id : -asset_id);
                }
                clause_assignments.insert(satisfying_assignment);
            }
        }
        
        if (verbose) {
            std::cout << "\nClause " << (clause_idx + 1) << ": '" << clause.name << "'" << std::endl;
            std::cout << "  Expression: " << clause.expression << std::endl;
            std::cout << "  Asset IDs: [";
            for (auto it = clause_asset_ids.begin(); it != clause_asset_ids.end(); ++it) {
                if (it != clause_asset_ids.begin()) std::cout << ", ";
                std::cout << *it;
            }
            std::cout << "]" << std::endl;
            std::cout << "  Satisfying assignments:" << std::endl;
            for (const auto& assignment : clause_assignments) {
                std::cout << "    [";
                for (size_t i = 0; i < assignment.size(); i++) {
                    if (i > 0) std::cout << ", ";
                    std::cout << assignment[i];
                }
                std::cout << "]" << std::endl;
            }
            std::cout << "  Total satisfying assignments: " << clause_assignments.size() << std::endl;
        }
        clause_satisfying_assignments.push_back(clause_assignments);
    }
    
    if (verbose) {
        std::cout << "\n=== SOLVER INTERFACE INPUT ===" << std::endl;
        std::cout << "Number of clauses: " << clause_satisfying_assignments.size() << std::endl;
        for (size_t i = 0; i < clause_satisfying_assignments.size(); i++) {
            std::cout << "Clause " << (i + 1) << " set size: " << clause_satisfying_assignments[i].size() << std::endl;
        }
        std::cout << "===============================" << std::endl;
    }
    
    // Export data for CUDA solver (CudaSet format)
    exportForCudaSolver(clause_satisfying_assignments, all_asset_ids);

    // --- JSON Export and CUDA Solver Execution ---
    if (verbose) {
        std::cout << "\n=== JSON EXPORT FOR CUDA ===" << std::endl;
    }
    std::ostringstream json;
    json << "{\n  \"assets\": [";
    for (size_t i = 0; i < asset_list.size(); ++i) {
        if (i > 0) json << ", ";
        json << asset_list[i];
    }
    json << "],\n  \"asset_names\": {";
    
    // Add asset name mapping for ZDD queries
    bool first_asset = true;
    for (size_t i = 0; i < asset_list.size(); ++i) {
        int asset_id = asset_list[i];
        // Find the asset name for this ID
        std::string asset_name = "unknown_asset_" + std::to_string(asset_id);
        for (const auto& pair : asset_to_id) {
            if (pair.second == asset_id) {
                asset_name = pair.first;
                break;
            }
        }
        if (!first_asset) json << ", ";
        json << "\"" << asset_id << "\": \"" << asset_name << "\"";
        first_asset = false;
    }
    json << "},\n  \"asset_construction\": {";
    
    // Add asset construction details (subject, action, object) for ZDD queries
    first_asset = true;
    for (size_t i = 0; i < asset_list.size(); ++i) {
        int asset_id = asset_list[i];
        // Find the asset name for this ID
        std::string asset_name = "unknown_asset_" + std::to_string(asset_id);
        for (const auto& pair : asset_to_id) {
            if (pair.second == asset_id) {
                asset_name = pair.first;
                break;
            }
        }
        
        // Get asset components from symbol table
        std::vector<std::string> components;
        auto it = symbol_table.find(asset_name);
        if (it != symbol_table.end() && it->second.type_keyword == "asset") {
            components = it->second.asset_components;
        }
        
        if (!first_asset) json << ", ";
        json << "\"" << asset_id << "\": {";
        json << "\"subject\": \"" << (components.size() > 0 ? components[0] : "unknown") << "\", ";
        json << "\"action\": \"" << (components.size() > 1 ? components[1] : "unknown") << "\", ";
        json << "\"object\": \"" << (components.size() > 2 ? components[2] : "unknown") << "\"";
        json << "}";
        first_asset = false;
    }
    json << "},\n  \"clauses\": [\n";
    for (size_t clause_idx = 0; clause_idx < clause_satisfying_assignments.size(); ++clause_idx) {
        const auto& clause = current_clauses[clause_idx];
        const auto& assignments = clause_satisfying_assignments[clause_idx];
        // Get the asset list for this clause
        std::set<int> clause_asset_ids;
        collectAssetIDs(clause.expr, clause_asset_ids);
        std::vector<int> clause_asset_list(clause_asset_ids.begin(), clause_asset_ids.end());
        json << "    {\n      \"name\": \"" << clause.name << "\",\n      \"asset_ids\": [";
        for (size_t i = 0; i < clause_asset_list.size(); ++i) {
            if (i > 0) json << ", ";
            json << clause_asset_list[i];
        }
        json << "],\n      \"assignments\": [\n";
        size_t assign_count = 0;
        for (const auto& assignment : assignments) {
            json << "        [";
            for (size_t j = 0; j < assignment.size(); ++j) {
                if (j > 0) json << ", ";
                json << assignment[j];
            }
            json << "]";
            assign_count++;
            if (assign_count < assignments.size()) json << ",";
            json << "\n";
        }
        json << "      ]\n    }";
        if (clause_idx < clause_satisfying_assignments.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ]\n}";
    
    // Generate unique filenames for this global check
    global_check_counter++;
    std::string json_filename = "witness_export_" + std::to_string(global_check_counter) + ".json";
    std::string result_filename = "zdd_" + std::to_string(global_check_counter) + ".bin";
    
    // Write JSON to file
    std::ofstream json_file(json_filename);
    if (json_file.is_open()) {
        json_file << json.str();
        json_file.close();
        if (verbose) {
            std::cout << "JSON exported to " << json_filename << std::endl;
        }
    } else {
        std::cerr << "Error: Could not write JSON to " << json_filename << std::endl;
        return;
    }
    
    // Call CUDA solver with output filename
    if (verbose) {
        std::cout << "\n=== CALLING CUDA SOLVER ===" << std::endl;
    }
    std::string cuda_command = "./tree_fold_cuda " + json_filename + " " + result_filename;
    if (verbose) {
        std::cout << "Executing: " << cuda_command << std::endl;
    }
    
    int result = system(cuda_command.c_str());
    if (result != 0) {
        std::cerr << "Error: CUDA solver returned exit code " << result << std::endl;
        return;
    }
    
    // Read results from CUDA solver
    if (verbose) {
        std::cout << "\n=== READING CUDA SOLVER RESULTS ===" << std::endl;
    }
    std::ifstream result_file(result_filename, std::ios::binary);
    if (!result_file.is_open()) {
        std::cerr << "Error: Could not open result file " << result_filename << std::endl;
        return;
    }
    
    std::vector<std::vector<int>> final_combinations;
    while (result_file.good()) {
        int size;
        result_file.read(reinterpret_cast<char*>(&size), sizeof(int));
        if (result_file.eof()) break;
        
        if (size > 0 && size <= 1000) { // Sanity check
            std::vector<int> combination(size);
            result_file.read(reinterpret_cast<char*>(combination.data()), size * sizeof(int));
            if (result_file.good()) {
                final_combinations.push_back(combination);
            }
        }
    }
    result_file.close();
    
    if (verbose) {
        std::cout << "CUDA solver found " << final_combinations.size() << " satisfying combinations" << std::endl;
        
        // Display first few results
        std::cout << "\n=== FIRST 10 SATISFYING COMBINATIONS ===" << std::endl;
        for (size_t i = 0; i < std::min(final_combinations.size(), size_t(10)); ++i) {
            std::cout << "Combination " << (i + 1) << ": [";
            for (size_t j = 0; j < final_combinations[i].size(); ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << final_combinations[i][j];
            }
            std::cout << "]" << std::endl;
        }
        
        if (final_combinations.size() > 10) {
            std::cout << "... and " << (final_combinations.size() - 10) << " more combinations" << std::endl;
        }
        
        std::cout << "=== END CUDA SOLVER RESULTS ===" << std::endl;
    }
}

} // namespace witness 