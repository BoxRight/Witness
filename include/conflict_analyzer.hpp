#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "clause_info.hpp"

namespace witness {

class ConflictAnalyzer {
public:
    ConflictAnalyzer();
    
    // Find minimal conflicting set of clauses
    std::vector<std::string> findMinimalConflictingSet(
        const std::vector<ClauseInfo>& clauses,
        const std::unordered_map<int, std::string>& asset_mapping);
    
    // Generate human-readable conflict report
    std::string generateConflictReport(
        const std::vector<std::string>& conflicting_clauses,
        const std::unordered_map<int, std::string>& asset_mapping);

private:
    // Analyze conflicts in binary operations
    std::vector<std::string> analyzeBinaryOperationConflicts(
        const ClauseInfo& clause,
        const std::vector<ClauseInfo>& all_clauses,
        const std::unordered_map<int, std::string>& asset_mapping);
    
    // Format clause description for reporting
    std::string formatClauseDescription(
        const ClauseInfo& clause,
        const std::unordered_map<int, std::string>& asset_mapping);
};

} // namespace witness 