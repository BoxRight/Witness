#pragma once

#include "ast.hpp"
#include <string>
#include <vector>

namespace witness {

// Data structure for clause information used in satisfiability checking
struct ClauseInfo {
    std::string name;                        // Clause name
    std::vector<int> positive_literals;      // Asset IDs that must be true
    std::vector<int> negative_literals;      // Asset IDs that must be false
    std::string expression;                  // Original expression string
    Expression* expr = nullptr;              // Pointer to the actual clause expression
};

} // namespace witness 