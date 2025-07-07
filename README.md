# Witness Programming Language

> **A semilattice-based programming language for legal reasoning and contract verification**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: Witness](https://img.shields.io/badge/Language-Witness-blue.svg)](#)
[![Status: Research](https://img.shields.io/badge/Status-Research-orange.svg)](#)

## Overview

**Witness** is a domain-specific programming language designed for modeling legal systems, contract validity, and procedural compliance. Built on mathematical foundations from lattice theory and constraint satisfaction, Witness enables developers and legal professionals to create executable specifications of legal relationships with guaranteed consistency.

### Key Features

- 🏗️ **Algebraic Foundation**: Built on semilattice operations with join-based composition
- ⚖️ **Legal Domain Modeling**: Native support for legal concepts, obligations, and procedural rules
- 🔍 **Satisfiability-Gated Computation**: Ensures legal consistency before proceeding with operations
- 🎯 **Contextual Join Operations**: Domain-specific operations for different legal relationship types
- 📊 **Truth Table Generation**: Automatic generation of satisfiability analyses
- 🎨 **Visual Representation**: Unified Hasse-ZDD diagrams for legal reasoning visualization

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/yourorg/witness-lang.git
cd witness-lang

# Build the compiler
make build

# Install system-wide
sudo make install
```

### Hello Legal World

Create a simple contract (`hello.witness`):

```witness
// Define types
object money = "payment amount", movable;
service payment = "financial payment", positive;
action pay = "make payment", payment;

subject buyer = "Alice";
subject seller = "Bob";

// Create assets
asset payment_obligation = buyer, pay, seller;

// Define validity
clause simple_contract = oblig(payment_obligation);

// Check satisfiability
asset contract_valid = global();
```

Compile and run:

```bash
witness --output=truth-table hello.witness
```

## Language Overview

### Core Concepts

**Assets** are the fundamental units representing legal relationships:
```witness
asset sale = seller, sell_house, buyer;
//    ^       ^        ^         ^
//    name    subject  action    subject
```

**Clauses** define logical relationships between assets:
```witness
clause contract = oblig(sale) IMPLIES oblig(payment);
```

**Global Operations** verify satisfiability and create witness assets:
```witness
asset valid_contract = global();  // Triggers satisfiability check
```

### Type System

Witness provides rich types for legal modeling:

```witness
// Objects with constraints
object house = "residential property", non_movable;
object cash = "payment currency", movable;

// Services with directionality  
service rent = "rental payment", positive;
service confidentiality = "non-disclosure", negative;

// Actions link to objects/services
action sell = "transfer ownership", house;
action pay = "provide payment", rent;

// Legal entities
subject tenant = "John Doe";
authority court = "Civil Court District 1";
time deadline = 30, within;
```

### Contextual Join Operations

Witness provides domain-specific operations that respect legal semantics:

```witness
// Commercial transactions
asset sale_transaction = sell(property_transfer, cash_payment);

// Service exchanges  
asset consulting_deal = compensation(legal_advice, accounting_service);

// Mixed obligations
asset employment_terms = consideration(salary_payment, non_compete_clause);

// Property rights
asset mortgage = encumber(house_ownership, loan_security);

// Evidence and argumentation
asset supported_claim = evidence(contract_breach, witness_testimony);
asset legal_argument = argument(supported_claim, statutory_authority);
```

## Advanced Features

### Satisfiability-Gated Computation

Witness ensures legal consistency through satisfiability checking:

```witness
// Basic contract clauses
clause offer = oblig(seller_offer);
clause acceptance = oblig(buyer_acceptance);
clause consideration = oblig(payment) AND oblig(delivery);

// Check if contract formation is satisfiable
asset contract_formation = global();

// Only if satisfiable, proceed with breach analysis
clause breach_analysis = contract_formation IMPLIES (
    not(payment) OR not(delivery)
);
```

### Evidence and Argument Construction

Build complex legal arguments through composition:

```witness
// Gather evidence
asset contract_document = notary, present_document, court;
asset witness_testimony = witness, testify, court;
asset expert_opinion = expert, provide_analysis, court;

// Build evidence chain
asset evidence_chain = evidence(
    evidence(contract_document, witness_testimony),
    expert_opinion
);

// Construct legal argument
asset breach_argument = argument(evidence_chain, breach_claim);

// Model adversarial proceedings
clause dispute = oblig(breach_argument) XOR oblig(defense_argument);
```

### Domain Operations

Aggregate related legal concepts:

```witness
// Analyze multiple related clauses
asset capacity_analysis = domain(
    procedural_capacity_valid,
    substantive_capacity_valid,
    temporal_capacity_valid
);
```

### Meet Operations

Extract common elements from complex legal structures:

```witness
asset common_obligations = meet(
    contract_obligations,
    statutory_obligations
);
```

## Compilation and Output

### Command Line Interface

```bash
# Basic compilation
witness contract.witness

# Output formats
witness --output=truth-table contract.witness    # Show satisfiability tables
witness --output=zdd contract.witness           # Generate ZDD files
witness --output=proof contract.witness         # Show proof witnesses only

# Error handling
witness --on-unsat=halt contract.witness        # Stop on unsatisfiable
witness --on-unsat=persist contract.witness     # Continue despite errors
witness --on-unsat=check contract.witness       # Check syntax first

# Verbose output
witness --verbose contract.witness
```

### Truth Table Output

```
TRUTH TABLE: contract_valid
offer | acceptance | payment | delivery
  1   |     1      |    1    |    1
  1   |     1      |    0    |    0
  0   |     0      |    0    |    0

TRUTH TABLE: breach_analysis
payment | delivery | breach_claim
   1    |    0     |     1
   0    |    1     |     1
   0    |    0     |     1
```

## Examples

### Contract Formation

See [`examples/contract_formation.witness`](examples/contract_formation.witness) for a complete contract formation example.

### Real Estate Transaction

See [`examples/real_estate.witness`](examples/real_estate.witness) for a comprehensive real estate purchase with breach analysis and litigation.

### Employment Agreement

See [`examples/employment.witness`](examples/employment.witness) for employment contracts with confidentiality and non-compete clauses.

### Litigation Procedure

See [`examples/litigation.witness`](examples/litigation.witness) for procedural validity checking and court proceedings.

## Mathematical Foundation

Witness is built on solid mathematical principles:

### Semilattice Theory
- **Universal Join Operation**: Forms a complete semilattice over all assets
- **Domain-Specific Semilattices**: Each contextual operation forms a semilattice within its domain
- **Idempotency**: `join(x, x) = x` for all assets
- **Associativity**: `join(join(a, b), c) = join(a, join(b, c))`
- **Commutativity**: `join(a, b) = join(b, a)`

### Constraint Satisfaction
- **Satisfiability Checking**: Ensures legal consistency before computation
- **Witness Generation**: Creates proof objects for satisfiable legal states
- **Error Detection**: Identifies inconsistent legal specifications

### Type Theory
- **Rich Type System**: Objects, services, actions with semantic constraints
- **Type Preservation**: All operations maintain type invariants
- **Contextual Constraints**: Domain-specific rules for legal operations

## Research and Academic Use

Witness represents a novel approach to computational law and legal informatics. Key research contributions:

1. **Algebraic Legal Modeling**: First language to treat legal relationships as algebraic structures
2. **Satisfiability-Gated Computation**: Ensures consistency before legal reasoning
3. **Contextual Join Semantics**: Domain-specific operations respecting legal semantics
4. **Visual Legal Reasoning**: Unified Hasse-ZDD diagrams for legal structure visualization

### Publications

- *"Witness: A Semilattice-Based Language for Legal Reasoning"* (forthcoming)
- *"Satisfiability-Gated Computation in Legal Programming"* (submitted)
- *"Algebraic Foundations for Computational Law"* (in preparation)

## Contributing

We welcome contributions from both programming language researchers and legal technology experts!

### Development Setup

```bash
# Clone and setup development environment
git clone https://github.com/yourorg/witness-lang.git
cd witness-lang
make dev-setup

# Run tests
make test

# Build documentation
make docs
```

### Contribution Guidelines

1. **Language Design**: Proposals for new legal constructs or operations
2. **Compiler Implementation**: Parser, type checker, satisfiability solver
3. **Standard Library**: Common legal patterns and templates
4. **Documentation**: Examples, tutorials, legal domain explanations
5. **Research**: Mathematical foundations, formal semantics, legal applications

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines.

## Roadmap

### Short Term (6 months)
- [ ] Complete compiler implementation
- [ ] Standard library of legal patterns
- [ ] IDE integration and syntax highlighting
- [ ] Truth table optimization algorithms

### Medium Term (1 year)
- [ ] Advanced satisfiability solvers (SMT integration)
- [ ] Visual diagram generation tools
- [ ] Legal template system
- [ ] Integration with legal databases

### Long Term (2+ years)
- [ ] Machine learning for legal pattern recognition
- [ ] Blockchain integration for smart contracts
- [ ] Natural language to Witness translation
- [ ] Formal verification tools for legal specifications

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Citation

If you use Witness in academic research, please cite:

```bibtex
@software{witness_lang_2025,
  title = {Witness: A Semilattice-Based Programming Language for Legal Reasoning},
  author = {[Author Names]},
  year = {2025},
  url = {https://github.com/yourorg/witness-lang},
  version = {0.1.0}
}
```

## Support and Community

- **Documentation**: [https://witness-lang.org/docs](https://witness-lang.org/docs)
- **Forum**: [https://forum.witness-lang.org](https://forum.witness-lang.org)
- **Discord**: [Witness Language Community](https://discord.gg/witness-lang)
- **Issues**: [GitHub Issues](https://github.com/yourorg/witness-lang/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourorg/witness-lang/discussions)

## Acknowledgments

- Mathematical foundations inspired by lattice theory and universal algebra
- Legal domain modeling informed by jurisprudential theory and legal informatics
- Constraint satisfaction techniques from automated theorem proving
- Visual representation concepts from knowledge graphs and decision diagrams

---

**Witness**: *Making legal reasoning algebraic, one join at a time.* ⚖️