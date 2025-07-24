#include "conflict_analyzer.hpp"
#include <iostream>
#include <vector>
#include <set>
#include <unordered_map>
#include <algorithm>

namespace witness {

ConflictAnalyzer::ConflictAnalyzer() {
    // Initialize the conflict analyzer
}

std::vector<std::string> ConflictAnalyzer::findMinimalConflictingSet(
    const std::vector<ClauseInfo>& clauses,
    const std::unordered_map<int, std::string>& asset_mapping) {
    
    std::vector<std::string> conflicting_clauses;
    
    if (clauses.empty()) {
        return conflicting_clauses;
    }
    
    // Strategy 1: Check for direct contradictions (same asset with opposite literals)
    std::unordered_map<int, std::set<std::string>> asset_to_clauses;
    std::unordered_map<int, bool> asset_positive_literal;
    std::unordered_map<int, bool> asset_negative_literal;
    
    // Build asset-to-clauses mapping and check for direct contradictions
    for (const auto& clause : clauses) {
        std::string clause_desc = formatClauseDescription(clause, asset_mapping);
        
        // Check positive literals
        for (int asset_id : clause.positive_literals) {
            asset_to_clauses[asset_id].insert(clause_desc);
            if (asset_negative_literal.find(asset_id) != asset_negative_literal.end()) {
                // Found contradiction: asset is both positive and negative
                conflicting_clauses.push_back(clause_desc);
                // Add the clause that has the negative literal
                for (const auto& other_clause : clauses) {
                    for (int neg_asset : other_clause.negative_literals) {
                        if (neg_asset == asset_id) {
                            std::string other_desc = formatClauseDescription(other_clause, asset_mapping);
                            if (std::find(conflicting_clauses.begin(), conflicting_clauses.end(), other_desc) == conflicting_clauses.end()) {
                                conflicting_clauses.push_back(other_desc);
                            }
                            break;
                        }
                    }
                }
            }
            asset_positive_literal[asset_id] = true;
        }
        
        // Check negative literals
        for (int asset_id : clause.negative_literals) {
            asset_to_clauses[asset_id].insert(clause_desc);
            if (asset_positive_literal.find(asset_id) != asset_positive_literal.end()) {
                // Found contradiction: asset is both positive and negative
                conflicting_clauses.push_back(clause_desc);
                // Add the clause that has the positive literal
                for (const auto& other_clause : clauses) {
                    for (int pos_asset : other_clause.positive_literals) {
                        if (pos_asset == asset_id) {
                            std::string other_desc = formatClauseDescription(other_clause, asset_mapping);
                            if (std::find(conflicting_clauses.begin(), conflicting_clauses.end(), other_desc) == conflicting_clauses.end()) {
                                conflicting_clauses.push_back(other_desc);
                            }
                            break;
                        }
                    }
                }
            }
            asset_negative_literal[asset_id] = true;
        }
    }
    
    // Strategy 2: Check for logical contradictions in binary operations
    for (const auto& clause : clauses) {
        if (clause.expression == "binary_op" && clause.expr) {
            std::vector<std::string> binary_conflicts = analyzeBinaryOperationConflicts(clause, clauses, asset_mapping);
            conflicting_clauses.insert(conflicting_clauses.end(), binary_conflicts.begin(), binary_conflicts.end());
        }
    }
    
    // Remove duplicates
    std::sort(conflicting_clauses.begin(), conflicting_clauses.end());
    conflicting_clauses.erase(std::unique(conflicting_clauses.begin(), conflicting_clauses.end()), conflicting_clauses.end());
    
    // If no direct contradictions found, add a note about complex interactions
    if (conflicting_clauses.empty()) {
        conflicting_clauses.push_back("No direct explicit contradictions detected");
        conflicting_clauses.push_back("Unsatisfiability may be due to complex logical interactions between clauses");
        conflicting_clauses.push_back("Consider reviewing clause dependencies and logical constraints");
    }
    
    return conflicting_clauses;
}

std::vector<std::string> ConflictAnalyzer::analyzeBinaryOperationConflicts(
    const ClauseInfo& clause,
    const std::vector<ClauseInfo>& all_clauses,
    const std::unordered_map<int, std::string>& asset_mapping) {
    
    std::vector<std::string> conflicts;
    
    // For now, we'll implement a simple heuristic for binary operations
    // This can be expanded to handle more complex logical contradictions
    
    // Check if this binary operation clause conflicts with other clauses
    // by analyzing the logical implications
    
    // Example: If we have clause1 = oblig(A) AND clause2 = not(A)
    // and this binary operation combines them, it creates a contradiction
    
    return conflicts;
}

std::string ConflictAnalyzer::formatClauseDescription(
    const ClauseInfo& clause,
    const std::unordered_map<int, std::string>& asset_mapping) {
    
    std::string desc = "clause '" + clause.name + "': " + clause.expression;
    
    // Add asset details if available
    std::vector<std::string> asset_details;
    
    for (int asset_id : clause.positive_literals) {
        auto it = asset_mapping.find(asset_id);
        if (it != asset_mapping.end()) {
            asset_details.push_back("oblig(" + it->second + ")");
        } else {
            asset_details.push_back("oblig(asset_" + std::to_string(asset_id) + ")");
        }
    }
    
    for (int asset_id : clause.negative_literals) {
        auto it = asset_mapping.find(asset_id);
        if (it != asset_mapping.end()) {
            asset_details.push_back("not(" + it->second + ")");
        } else {
            asset_details.push_back("not(asset_" + std::to_string(asset_id) + ")");
        }
    }
    
    if (!asset_details.empty()) {
        desc += " [";
        for (size_t i = 0; i < asset_details.size(); ++i) {
            if (i > 0) desc += ", ";
            desc += asset_details[i];
        }
        desc += "]";
    }
    
    return desc;
}

std::string ConflictAnalyzer::generateConflictReport(
    const std::vector<std::string>& conflicting_clauses,
    const std::unordered_map<int, std::string>& asset_mapping) {
    
    if (conflicting_clauses.empty()) {
        return "No conflicts detected.";
    }
    
    // Check if we have actual conflicts or just informational messages
    bool has_actual_conflicts = false;
    for (const auto& clause : conflicting_clauses) {
        if (clause.find("No direct explicit contradictions") == std::string::npos &&
            clause.find("Unsatisfiability may be due to") == std::string::npos &&
            clause.find("Consider reviewing") == std::string::npos) {
            has_actual_conflicts = true;
            break;
        }
    }
    
    std::string report = "Error: Unsatisfiable clauses detected\n\n";
    
    if (has_actual_conflicts) {
        report += "Minimal conflicting set:\n";
        for (size_t i = 0; i < conflicting_clauses.size(); ++i) {
            report += "  " + std::to_string(i + 1) + ". " + conflicting_clauses[i] + "\n";
        }
        
        // Add assets involved
        std::set<int> involved_assets;
        for (const auto& clause_desc : conflicting_clauses) {
            // Extract asset IDs from clause descriptions
            // This is a simplified extraction - could be made more robust
            for (const auto& asset_pair : asset_mapping) {
                if (clause_desc.find(asset_pair.second) != std::string::npos) {
                    involved_assets.insert(asset_pair.first);
                }
            }
        }
        
        if (!involved_assets.empty()) {
            report += "\nAssets involved:\n";
            for (int asset_id : involved_assets) {
                auto it = asset_mapping.find(asset_id);
                if (it != asset_mapping.end()) {
                    report += "  - " + it->second + " (ID: " + std::to_string(asset_id) + ")\n";
                }
            }
        }
        
        report += "\nSuggestion: Review conflicting obligations in your contract specification.";
    } else {
        report += "Analysis Results:\n";
        for (size_t i = 0; i < conflicting_clauses.size(); ++i) {
            report += "  " + std::to_string(i + 1) + ". " + conflicting_clauses[i] + "\n";
        }
        report += "\nSuggestion: The system is unsatisfiable due to complex logical interactions. Consider simplifying clause dependencies or reviewing the overall contract structure.";
    }
    
    return report;
}

} // namespace witness 