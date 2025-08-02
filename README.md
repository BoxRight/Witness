# Witness Programming Language

> **A domain-specific language for legal reasoning with satisfiability checking and CUDA acceleration**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: Witness](https://img.shields.io/badge/Language-Witness-blue.svg)](#)
[![Status: Active Development](https://img.shields.io/badge/Status-Active%20Development-green.svg)](#)

## Overview

**Witness** is a domain-specific programming language designed for modeling legal systems, contract validity, and procedural compliance. Built on mathematical foundations from lattice theory and constraint satisfaction, Witness enables developers and legal professionals to create executable specifications of legal relationships with guaranteed consistency.

### Key Features

- ⚖️ **Legal Domain Modeling**: Native support for legal concepts, obligations, and procedural rules
- 🔍 **Satisfiability Checking**: Built-in exhaustive and CUDA-accelerated satisfiability solvers
- 🎯 **System Operations**: `global()`, `litis()`, `meet()`, and `domain()` for legal analysis
- 📊 **Conflict Analysis**: Automatic detection of minimal conflicting sets in unsatisfiable clauses
- 🚀 **CUDA Acceleration**: GPU-accelerated satisfiability checking for large contracts
- 🎨 **Verbosity Control**: Clean output with `--quiet` and `--verbose` flags

## Quick Start

### Prerequisites

Before building Witness, ensure you have the following tools installed:

#### Required Tools
- **GCC/G++** (C++17 support)
- **Make** (GNU Make)
- **Bison** (Parser generator)
- **Flex** (Lexical analyzer)

#### Installation by Platform

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential bison flex
```

**CentOS/RHEL/Fedora:**
```bash
sudo yum install gcc-c++ make bison flex
# or for newer systems:
sudo dnf install gcc-c++ make bison flex
```

**macOS:**
```bash
# Using Homebrew
brew install bison flex

# Using MacPorts
sudo port install bison flex
```

**Arch Linux/Manjaro:**
```bash
sudo pacman -S base-devel bison flex
```

**Windows (WSL):**
```bash
sudo apt-get update
sudo apt-get install build-essential bison flex
```

#### Optional: CUDA Support
For CUDA acceleration, you'll also need:
- **NVIDIA GPU** with CUDA support
- **CUDA Toolkit** (version 11.0 or later)
- **nvcc** compiler

### Installation

```bash
# Clone the repository
git clone https://github.com/BoxRight/Witness.git
cd Witness

# Build the compiler
make clean && make

# Optional: Build CUDA solver (requires NVIDIA GPU and CUDA toolkit)
nvcc -arch=sm_86 -std=c++17 tree_fold_cuda.cu -o tree_fold_cuda -ltbb -lzstd
```

### Hello Legal World

Create a simple contract (`hello.wit`):

```wit
// Define assets
asset bike_sale = alice, sell_bike, bob;
asset payment = bob, pay, alice;

// Define clauses
clause clause1 = oblig(bike_sale);
clause clause2 = oblig(payment);

// Check satisfiability
asset contract_valid = global();
```

Compile and run:

```bash
./witnessc hello.wit
```

## Language Overview

### Core Concepts

**Assets** are the fundamental units representing legal relationships:
```wit
asset bike_sale = alice, sell_bike, bob;
//    ^          ^       ^          ^
//    name       subject action     subject
```

**Clauses** define logical relationships between assets:
```wit
clause contract = oblig(bike_sale) IMPLIES oblig(payment);
```

**System Operations** verify satisfiability and create witness assets:
```wit
asset valid_contract = global();  // Triggers satisfiability check
```

### System Operations

Witness provides powerful system operations for legal analysis:

```wit
// Global satisfiability check
asset global_check = global();

// Selective satisfiability check for specific assets
asset litis_check = litis(bike_sale, payment);

// Greatest common legal denominator extraction
asset meet_result = meet(contract1, contract2);

// Domain analysis for completeness verification
asset domain_check = domain(asset1, asset2, asset3);
```

### Logical Operations

Witness provides logical operations for legal reasoning:

```wit
// Obligations
clause obligation = oblig(bike_sale);

// Claims
clause claim = claim(payment);

// Negation
clause prohibition = not(oblig(bike_sale));

// Binary operations
clause conditional = oblig(bike_sale) IMPLIES oblig(payment);
clause conjunction = oblig(bike_sale) AND oblig(payment);
clause disjunction = oblig(bike_sale) OR oblig(payment);
clause equivalence = oblig(bike_sale) EQUIV oblig(payment);
```

## Advanced Features

### Satisfiability Checking

Witness provides two satisfiability checking modes:

#### Exhaustive Solver (Default)
```bash
./witnessc test_file.wit
```

#### CUDA-Accelerated Solver
```bash
./witnessc --solver=external test_file.wit
```

### Conflict Analysis

When clauses are unsatisfiable, Witness automatically detects minimal conflicting sets:

```wit
// Conflicting clauses
clause clause1 = oblig(bike_sale);
clause clause2 = not(oblig(bike_sale));  // Direct contradiction

asset check = global();  // Will trigger conflict analysis
```

Output:
```
Global check UNSATISFIABLE
Minimal conflicting set:
- Clause 'clause1': oblig(bike_sale)
- Clause 'clause2': not(oblig(bike_sale))

Analysis Results:
Direct contradiction detected between:
- Asset 'bike_sale' (positive in clause1, negative in clause2)
```

### Verbosity Control

Witness provides flexible output control:

```bash
# Default output (shows warnings and basic info)
./witnessc test_file.wit

# Quiet mode (only errors and results)
./witnessc --quiet test_file.wit

# Verbose mode (shows AST, debug info, detailed assignments)
./witnessc --verbose test_file.wit
```

### Mixed Operations

You can mix different system operations:

```wit
// First global check
clause clause1 = oblig(bike_sale);
clause clause2 = oblig(payment);
asset global_check1 = global();

// Then litis check (clause set is reset)
clause clause3 = not(oblig(bike_sale));
asset litis_check = litis(bike_sale, payment);

// Another global check (clause set is reset again)
clause clause4 = oblig(bike_sale);
asset global_check2 = global();
```

### Join Operations

Witness supports various join operations for legal relationships:

```wit
// Universal joins
asset universal_join = join(asset1, asset2);

// Contextual joins (require reciprocal patterns)
asset transfer_join = transfer(asset1, asset2);
asset sell_join = sell(asset1, asset2);
asset compensation_join = compensation(asset1, asset2);
asset consideration_join = consideration(asset1, asset2);
asset forbearance_join = forbearance(asset1, asset2);
asset encumber_join = encumber(asset1, asset2);
asset access_join = access(asset1, asset2);
asset lien_join = lien(asset1, asset2);
asset evidence_join = evidence(asset1, asset2);
asset argument_join = argument(asset1, asset2);
```

**Note**: Contextual joins require reciprocal subject patterns: `(s1,A1,s2) ↔ (s2,A2,s1)`

## Compilation and Output

### Command Line Interface

```bash
# Basic compilation
./witnessc contract.wit

# Solver modes
./witnessc --solver=exhaustive contract.wit     # Use built-in exhaustive solver (default)
./witnessc --solver=external contract.wit       # Use CUDA-accelerated solver

# Verbosity control
./witnessc --quiet contract.wit                 # Suppress warnings and debug output
./witnessc --verbose contract.wit               # Show detailed output including AST

# Help
./witnessc --help                               # Show usage information
```

### Sample Output

#### Satisfiable Case
```bash
$ ./witnessc --quiet test_simple.wit
Global check SATISFIABLE
```

#### Unsatisfiable Case with Conflict Analysis
```bash
$ ./witnessc --quiet test_conflicts.wit
Global check UNSATISFIABLE
Minimal conflicting set:
- Clause 'clause1': oblig(bike_sale)
- Clause 'clause2': not(oblig(bike_sale))

Analysis Results:
Direct contradiction detected between:
- Asset 'bike_sale' (positive in clause1, negative in clause2)
```

#### Verbose Output
```bash
$ ./witnessc --verbose test_file.wit
Using solver mode: exhaustive
Parsing successful!

--- AST ---
[AST structure displayed]

Running semantic analysis...
Warning: Asset 'bike_sale' assigned ID 1 for satisfiability checking
Warning: Clause 'clause1' added: [+1 ] from 'oblig(bike_sale)'
Global check SATISFIABLE
```

## Examples

### Basic Contract
See [`examples/example.wit`](examples/example.wit) for a simple contract example.

### Conflict Analysis
See [`test_conflicts.wit`](test_conflicts.wit) for examples of conflicting clauses and automatic conflict detection.

### System Operations
See [`test_litis.wit`](test_litis.wit) for examples of `litis()` selective satisfiability checking.

### Mixed Operations
See [`test_mixed_operations.wit`](test_mixed_operations.wit) for examples of mixing `global()` and `litis()` operations.

### Complex Solar Panel Contract
See [`test_solar_panels.wit`](test_solar_panels.wit) for a comprehensive modular solar lease contract with multiple clauses and system operations.

## Implementation Details

### Architecture

The Witness compiler is built with:

- **Parser**: Bison-based parser with Flex lexer
- **Semantic Analyzer**: C++ implementation with satisfiability checking
- **Conflict Analyzer**: Automatic detection of minimal conflicting sets
- **CUDA Solver**: GPU-accelerated satisfiability checking for large contracts
- **JSON Export**: Standard format for external solver communication

### Satisfiability Checking

Two modes are supported:

1. **Exhaustive Solver**: Built-in solver that generates all possible truth assignments
2. **CUDA Solver**: External GPU-accelerated solver using tree-fold algorithm

### File Formats

- **Input**: `.wit` files with Witness DSL syntax
- **Intermediate**: JSON export format for CUDA solver communication
- **Output**: Binary format for CUDA solver results

## Current Status

### Implemented Features

✅ **Core Language**: Asset definitions, clauses, logical operations  
✅ **System Operations**: `global()`, `litis()`  
✅ **Satisfiability Checking**: Exhaustive and CUDA-accelerated solvers  
✅ **Conflict Analysis**: Automatic detection of minimal conflicting sets  
✅ **Verbosity Control**: `--quiet` and `--verbose` flags  
✅ **Join Operations**: Universal and contextual joins with validation  
✅ **Error Handling**: Comprehensive error reporting and validation  

### In Progress

🔄 **System Operations**: `meet()` and `domain()` implementations  
🔄 **Advanced Join Validation**: Enhanced reciprocal pattern checking  
🔄 **Performance Optimization**: CUDA solver improvements  

### Planned Features

📋 **Visualization**: Truth table and conflict diagram generation  
📋 **Standard Library**: Common legal patterns and templates  
📋 **IDE Integration**: Syntax highlighting and error checking  
📋 **Documentation**: Comprehensive language reference

## Contributing

We welcome contributions from both programming language researchers and legal technology experts!

### Development Setup

```bash
# Clone the repository
git clone https://github.com/BoxRight/Witness.git
cd Witness

# Build the compiler
make clean && make

# Run tests
./witnessc test_conflicts.wit
./witnessc test_litis.wit
./witnessc test_mixed_operations.wit
```

### Contribution Areas

1. **Language Features**: New system operations, logical constructs
2. **Solver Improvements**: Enhanced satisfiability checking algorithms
3. **Performance**: CUDA optimization, parallel processing
4. **Documentation**: Examples, tutorials, language reference
5. **Testing**: Test cases, validation scenarios

### Building from Source

Requirements:
- C++17 compiler (GCC 7+ or Clang 5+)
- Bison and Flex
- Make
- Optional: CUDA toolkit for GPU acceleration

## Roadmap

### Short Term (3 months)
- [x] Core language implementation
- [x] Basic satisfiability checking
- [x] Conflict analysis
- [x] CUDA integration
- [ ] Complete system operations (`meet()`, `domain()`)
- [ ] Enhanced error reporting

### Medium Term (6 months)
- [ ] Visual output generation (truth tables, diagrams)
- [ ] Standard library of legal patterns
- [ ] Performance optimization
- [ ] Comprehensive test suite

### Long Term (1 year)
- [ ] IDE integration and syntax highlighting
- [ ] Advanced satisfiability solvers
- [ ] Legal template system
- [ ] Documentation and tutorials

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Citation

If you use Witness in academic research, please cite:

```bibtex
@software{witness_lang_2025,
  title = {Witness: A Semilattice-Based Programming Language for Legal Reasoning},
  author = {[Author Names]},
  year = {2025},
  url = {https://github.com/BoxRight/Witness},
  version = {0.1.0}
}
```

## Support and Community

- **Issues**: [GitHub Issues](https://github.com/BoxRight/Witness/issues)
- **Discussions**: [GitHub Discussions](https://github.com/BoxRight/Witness/discussions)
- **Documentation**: This README and inline code comments

## Acknowledgments

- Mathematical foundations inspired by satisfiability checking and constraint satisfaction
- Legal domain modeling informed by legal informatics and computational law
- CUDA acceleration techniques from parallel computing and GPU programming
- Conflict analysis concepts from automated theorem proving and debugging

---

**Witness**: *Making legal reasoning computational, one clause at a time.* ⚖️