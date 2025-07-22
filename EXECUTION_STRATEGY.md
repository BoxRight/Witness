# Witness Execution Strategy and Implementation Plan

> **Comprehensive plan for implementing satisfiability checking, truth table generation, and execution engine**

## Overview

This document outlines the complete execution strategy for the Witness programming language, focusing on the satisfiability checking engine, truth table generation, and the integration of assets, clauses, and global operations.

## Architecture Overview

```
Source Code
     ↓
Semantic Analysis (Type Checking, Join Validation)
     ↓
Lazy Asset ID Assignment (Only for assets used in clauses)
     ↓
Clause Collection & Truth Table Generation
     ↓
Satisfiability Checking (Exhaustive or SAT Solver)
     ↓
ZDD Output Generation
     ↓
Witness Certificate Creation
```

## Key Design Decisions

### 1. **Lazy Asset ID Assignment**
- **Strategy**: Single-pass lazy assignment - IDs assigned only when assets appear in clauses
- **Benefits**: No wasted IDs, simpler implementation, better performance
- **Implementation**: Hash map `asset_name → ID` with incremental counter

### 2. **Signed Literal Format**
- **Format**: `[+ID, -ID, +ID, ...]` where `+ID` = true, `-ID` = false
- **Example**: For `oblig(payment) AND not(breach)` with `payment=1, breach=2`: `[1, -2]`
- **Benefits**: Standard SAT format, compact representation

### 3. **Clause Reset Strategy**
- **Behavior**: Each `global()` call checks only clauses since the last `global()`
- **State Management**: Clause list resets after each satisfiability check
- **Benefits**: Incremental analysis, cleaner semantics

### 4. **Flexible Backend**
- **Command Flag**: `--solver=exhaustive` or `--solver=sat`
- **Exhaustive**: Generate complete truth table for small problems
- **SAT**: Use external SAT solver for large problems
- **Configuration**: External SAT solver via JSON config file

## Implementation TODOs

### **Phase 1: Core Execution Engine**

#### 🔧 **Lazy Asset ID Assignment** `[Priority: High]`
- **File**: `src/semantic_analyzer.cpp`
- **Task**: Implement lazy ID assignment during clause processing
- **Data Structure**: `std::unordered_map<std::string, int> asset_to_id`
- **Function**: `int getOrAssignAssetID(const std::string& asset_name)`
- **Integration**: Call during `oblig()`, `claim()`, `not()` processing

#### 🚀 **Command Line Flags** `[Priority: High]`
- **File**: `src/main.cpp`
- **Flags**: 
  - `--solver=exhaustive|sat` (default: exhaustive)
  - `--output=zdd|truth-table|both` (default: both)
  - `--timeout=30` (SAT solver timeout in seconds)
  - `--config=path/to/sat_config.json` (SAT solver config)
- **Integration**: Parse in main, pass to semantic analyzer

#### 📊 **Truth Table Generation** `[Priority: High]`
- **File**: `src/truth_table_generator.cpp` (new)
- **Input**: List of clauses with asset IDs
- **Output**: `std::vector<std::vector<int>>` (satisfying assignments)
- **Format**: Each inner vector is a satisfying assignment with signed literals
- **Algorithm**: Exhaustive enumeration for 2^n combinations

### **Phase 2: SAT Solver Integration**

#### 🔌 **SAT Solver Integration** `[Priority: Medium]`
- **File**: `src/sat_solver_interface.cpp` (new)
- **Input Format**: DIMACS CNF format
- **Output Processing**: Parse SAT solver output back to signed literals
- **Error Handling**: Timeout, solver crashes, unsatisfiable results
- **Integration**: Call external process with proper I/O handling

#### ⚙️ **SAT Solver Configuration** `[Priority: Medium]`
- **File**: `sat_config.json` (new)
- **Format**: 
  ```json
  {
    "solver_path": "/usr/bin/minisat",
    "args": ["-verb=0", "-cpu-lim=30"],
    "timeout": 30,
    "temp_dir": "/tmp/witness"
  }
  ```
- **Loading**: JSON parser in `src/config_loader.cpp`

### **Phase 3: Output Generation**

#### 📈 **ZDD Output Generation** `[Priority: Medium]`
- **File**: `src/zdd_generator.cpp` (new)
- **Input**: Satisfying assignments from truth table or SAT solver
- **Output**: Zero-suppressed Decision Diagram file
- **Format**: Custom binary format or DOT graph for visualization
- **Integration**: Called after successful satisfiability check

#### 📜 **Witness Certificate Structure** `[Priority: Medium]`
- **File**: `src/witness_certificate.cpp` (new)
- **Contents**:
  - Satisfying assignment: `std::vector<int>`
  - Clause count: `int`
  - Asset mapping: `std::unordered_map<int, std::string>`
  - Metadata: timestamp, solver used, execution time
- **Serialization**: JSON format for human readability

### **Phase 4: Integration & Error Handling**

#### 🔄 **Clause Reset Mechanism** `[Priority: High]`
- **File**: `src/semantic_analyzer.cpp`
- **Implementation**: Clear clause list after each `global()` call
- **Data Structure**: `std::vector<ClauseInfo> current_clauses`
- **Reset Point**: After successful satisfiability check

#### 🌐 **Global Operation Integration** `[Priority: High]`
- **File**: `src/semantic_analyzer.cpp`
- **Task**: Connect `global()` system operation to execution engine
- **Flow**: 
  1. Trigger satisfiability check
  2. Generate truth table/call SAT solver
  3. Create witness certificate
  4. Reset clause list
- **Return**: Witness certificate asset

#### ⚠️ **Minimal Conflicting Set** `[Priority: Medium]`
- **File**: `src/conflict_analyzer.cpp` (new)
- **Algorithm**: Find minimal unsatisfiable core
- **Output**: Human-readable error message with conflicting clauses
- **Example**: `"Conflict: oblig(payment) conflicts with not(payment) in clauses C1, C3"`

### **Phase 5: Advanced Features**

#### 🔍 **Associativity Validation** `[Priority: Low]`
- **File**: `src/semantic_analyzer.cpp`
- **Task**: Validate `join(join(a,b), c) = join(a, join(b,c))`
- **Implementation**: Check nested join expressions during semantic analysis

#### 🎯 **Parser Enhancements** `[Priority: Low]`
- **File**: `src/parser.ypp`
- **Task**: Add formal parser support for logical operations
- **Operations**: `oblig`, `claim`, `not`, `prohibition`
- **Integration**: Currently handled as function calls, formalize as operators

#### ⚡ **Performance Optimization** `[Priority: Low]`
- **File**: `src/semantic_analyzer.cpp`
- **Tasks**:
  - Optimize symbol table lookups
  - Cache type checking results
  - Reduce memory allocation in large programs

## Data Structures

### **AssetInfo**
```cpp
struct AssetInfo {
    int id;                          // Unique asset identifier
    std::string name;                // Asset name
    std::vector<std::string> components; // [subject, action, object]
    bool used_in_clause;             // Whether asset appears in any clause
};
```

### **ClauseInfo**
```cpp
struct ClauseInfo {
    std::string name;                // Clause name
    std::vector<int> positive_literals; // Asset IDs that must be true
    std::vector<int> negative_literals; // Asset IDs that must be false
    std::string expression;          // Original expression string
};
```

### **SatisfiabilityResult**
```cpp
struct SatisfiabilityResult {
    bool satisfiable;
    std::vector<std::vector<int>> assignments; // All satisfying assignments
    std::string error_message;       // If unsatisfiable
    std::vector<std::string> conflicting_clauses; // Minimal conflict set
};
```

### **WitnessCertificate**
```cpp
struct WitnessCertificate {
    std::vector<int> satisfying_assignment;
    int clause_count;
    std::unordered_map<int, std::string> asset_mapping;
    std::string timestamp;
    std::string solver_used;
    double execution_time_ms;
};
```

## Command Line Interface

### **Basic Usage**
```bash
# Default: exhaustive solver with both outputs
witnessc contract.wit

# Specify solver
witnessc --solver=exhaustive contract.wit
witnessc --solver=sat contract.wit

# Specify output format
witnessc --output=zdd contract.wit
witnessc --output=truth-table contract.wit
witnessc --output=both contract.wit

# Advanced options
witnessc --solver=sat --timeout=60 --config=minisat.json contract.wit
```

### **Output Files**
- `contract.truth` - Truth table output
- `contract.zdd` - ZDD diagram file
- `contract.certificate` - Witness certificate JSON
- `contract.log` - Execution log

## Error Handling Strategy

### **Unsatisfiable Clauses**
```
Error: Unsatisfiable clauses detected

Minimal conflicting set:
  - oblig(payment) in clause 'contract_terms'
  - not(payment) in clause 'breach_condition'

Assets involved:
  - payment (ID: 1): buyer, pay_money, seller

Suggestion: Review conflicting obligations in your contract specification.
```

### **SAT Solver Errors**
```
Error: SAT solver failed

Solver: /usr/bin/minisat
Exit code: 1
Stderr: timeout after 30 seconds

Suggestion: Try increasing timeout with --timeout=60 or use --solver=exhaustive for smaller problems.
```

## Integration Points

### **Semantic Analyzer Integration**
- **ID Assignment**: During `validateLogicalOperation()`
- **Clause Collection**: During `oblig()`, `claim()`, `not()` processing
- **Global Processing**: During `validateSystemOperation()` for `global()`

### **Symbol Table Integration**
- **Asset Storage**: Extend existing `TypeInfo` structure
- **ID Mapping**: Add `asset_to_id` map to `SemanticAnalyzer`
- **Clause Storage**: Add `current_clauses` vector to `SemanticAnalyzer`

### **File System Integration**
- **Temporary Files**: SAT solver DIMACS input/output
- **Output Files**: Truth tables, ZDD diagrams, certificates
- **Configuration**: JSON config file loading

## Implementation Phases

### **Phase 1: Foundation** `[Weeks 1-2]`
- Lazy asset ID assignment
- Basic command line flags
- Simple truth table generation
- Clause reset mechanism

### **Phase 2: SAT Integration** `[Weeks 3-4]`
- External SAT solver interface
- DIMACS format generation
- Configuration file system
- Basic error handling

### **Phase 3: Advanced Features** `[Weeks 5-6]`
- ZDD output generation
- Witness certificates
- Minimal conflicting set analysis
- Advanced error reporting

### **Phase 4: Polish** `[Weeks 7-8]`
- Performance optimization
- Comprehensive testing
- Documentation updates
- Integration testing

## Testing Strategy

### **Unit Tests**
- Asset ID assignment correctness
- Truth table generation accuracy
- SAT solver integration robustness
- Clause reset behavior

### **Integration Tests**
- End-to-end satisfiability checking
- Multiple `global()` calls
- Large program performance
- Error handling completeness

### **Legal Domain Tests**
- Contract formation scenarios
- Breach analysis cases
- Complex join operations
- Real-world legal examples

---

*This execution strategy provides a comprehensive roadmap for implementing the satisfiability checking engine and execution framework for the Witness programming language.* 