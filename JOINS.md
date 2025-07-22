# Witness Operations Reference

This document provides a comprehensive reference for all operations available in the Witness programming language.

## Overview

Witness provides four categories of operations:
1. **Universal Join**: Works on all assets regardless of type
2. **Contextual Joins**: Domain-specific operations with type constraints and semantic rules
3. **System Operations**: Global analysis and domain-specific aggregation operations
4. **Logical Operations**: Logical constructs for building legal clauses and assertions

All join operations maintain the fundamental algebraic properties:
- **Idempotency**: `join(x, x) = x` (via symbol table identity)
- **Associativity**: `join(join(a,b), c) = join(a, join(b,c))`
- **Commutativity**: `join(a, b) = join(b, a)`
- **Closure**: Always produce valid assets `(subject, action, subject)`

## Universal Join Operation

### `join(asset1, asset2)`

**Domain**: All assets (universal)  
**Pattern**: No constraints  
**Type Checking**: None  
**Semantics**: Forceful combination regardless of content

#### Rules
```witness
join(asset_a, asset_b):
    if asset_a.symbol == asset_b.symbol:
        return asset_a  // Symbol-based idempotency
    else:
        return (asset_a.s1, "asset_a.action_asset_b.action", asset_a.s2, new_symbol)
```

#### Example
```witness
asset payment = buyer, pay_money, seller;
asset delivery = vendor, deliver_goods, customer;

asset combined = join(payment, delivery);
// Result: (buyer, "pay_money_deliver_goods", seller)
```

#### Use Cases
- Fallback operation when contextual joins don't apply
- Creating abstract combined concepts
- Forcing combinations for analysis purposes

---

## Object-Based Contextual Joins

### `transfer(asset1, asset2)`

**Domain**: Movable object actions ↔ Movable object actions  
**Pattern**: Reciprocal - `(s1, A1, s2)` and `(s2, A2, s1)`  
**Type Checking**: Both actions must operate on movable objects  
**Semantics**: Physical transfer of ownership or possession

#### Rules
```witness
transfer((s1, A1, s2), (s2, A2, s1)):
    if A1.object.constraint == movable AND A2.object.constraint == movable:
        return (s1, "transfer_A1.object_via_A2", s2)
    else:
        ERROR("transfer requires movable objects")
```

#### Example
```witness
object car = "automobile", movable;
action sell_car = "sell", car;
action deliver_car = "deliver", car;

asset sale = seller, sell_car, buyer;      // (seller, sell_car, buyer)
asset delivery = buyer, deliver_car, seller; // (buyer, deliver_car, seller)

asset car_transfer = transfer(sale, delivery);
// Result: (seller, "transfer_car_via_deliver_car", buyer)
```

#### Error Conditions
- Non-reciprocal pattern: `(s1, A1, s2)` and `(s3, A2, s4)` where s1≠s4 or s2≠s3
- Non-movable objects: Either action operates on non_movable object
- Non-object actions: Either action operates on service instead of object

### `sell(asset1, asset2)`

**Domain**: Object action ↔ Positive service action  
**Pattern**: Reciprocal - `(s1, A1, s2)` and `(s2, A2, s1)`  
**Type Checking**: A1 = object action, A2 = positive service action  
**Semantics**: Commercial exchange of object for service

#### Rules
```witness
sell((s1, A1, s2), (s2, A2, s1)):
    if A1.type == object_action AND A2.service.constraint == positive:
        return (s1, "sold_A1.object_for_A2.service", s2)
    else:
        ERROR("sell requires object action and positive service action")
```

#### Example
```witness
object house = "residential property", non_movable;
service payment = "purchase price", positive;
action sell_house = "sell", house;
action pay_price = "pay", payment;

asset property_sale = seller, sell_house, buyer;    // (seller, sell_house, buyer)
asset price_payment = buyer, pay_price, seller;     // (buyer, pay_price, seller)

asset house_sale = sell(property_sale, price_payment);
// Result: (seller, "sold_house_for_payment", buyer)
```

#### Error Conditions
- Wrong action types: A1 not object action or A2 not service action
- Negative service: A2 operates on negative service
- Non-reciprocal pattern

---

## Service-Based Contextual Joins

### `compensation(asset1, asset2)`

**Domain**: Positive service ↔ Positive service  
**Pattern**: Reciprocal - `(s1, A1, s2)` and `(s2, A2, s1)`  
**Type Checking**: Both actions must operate on positive services  
**Semantics**: Mutual service exchange

#### Rules
```witness
compensation((s1, A1, s2), (s2, A2, s1)):
    if A1.service.constraint == positive AND A2.service.constraint == positive:
        return (s1, "compensated_A1.service_with_A2.service", s2)
    else:
        ERROR("compensation requires positive services")
```

#### Example
```witness
service legal_advice = "legal consultation", positive;
service accounting = "financial analysis", positive;
action provide_legal = "provide", legal_advice;
action provide_accounting = "provide", accounting;

asset legal_service = lawyer, provide_legal, accountant;      // (lawyer, provide_legal, accountant)
asset accounting_service = accountant, provide_accounting, lawyer; // (accountant, provide_accounting, lawyer)

asset service_exchange = compensation(legal_service, accounting_service);
// Result: (lawyer, "compensated_legal_advice_with_accounting", accountant)
```

### `consideration(asset1, asset2)`

**Domain**: Positive service ↔ Negative service  
**Pattern**: Reciprocal - `(s1, A1, s2)` and `(s2, A2, s1)`  
**Type Checking**: A1 = positive service action, A2 = negative service action  
**Semantics**: Active obligation for omission obligation

#### Rules
```witness
consideration((s1, A1, s2), (s2, A2, s1)):
    if A1.service.constraint == positive AND A2.service.constraint == negative:
        return (s1, "A1.service_for_A2.service", s2)
    else:
        ERROR("consideration requires positive and negative services")
```

#### Example
```witness
service salary = "monthly payment", positive;
service non_compete = "competition restriction", negative;
action pay_salary = "pay", salary;
action abstain_compete = "abstain", non_compete;

asset salary_payment = employer, pay_salary, employee;        // (employer, pay_salary, employee)
asset non_compete_clause = employee, abstain_compete, employer; // (employee, abstain_compete, employer)

asset employment_deal = consideration(salary_payment, non_compete_clause);
// Result: (employer, "salary_for_non_compete", employee)
```

### `forbearance(asset1, asset2)`

**Domain**: Negative service ↔ Negative service  
**Pattern**: Reciprocal - `(s1, A1, s2)` and `(s2, A2, s1)`  
**Type Checking**: Both actions must operate on negative services  
**Semantics**: Mutual abstention agreement

#### Rules
```witness
forbearance((s1, A1, s2), (s2, A2, s1)):
    if A1.service.constraint == negative AND A2.service.constraint == negative:
        return (s1, "mutual_forbearance_A1.service_A2.service", s2)
    else:
        ERROR("forbearance requires negative services")
```

#### Example
```witness
service confidentiality_a = "non-disclosure A", negative;
service confidentiality_b = "non-disclosure B", negative;
action maintain_secrecy_a = "maintain", confidentiality_a;
action maintain_secrecy_b = "maintain", confidentiality_b;

asset secrecy_a = party_a, maintain_secrecy_a, party_b;  // (party_a, maintain_secrecy_a, party_b)
asset secrecy_b = party_b, maintain_secrecy_b, party_a;  // (party_b, maintain_secrecy_b, party_a)

asset mutual_secrecy = forbearance(secrecy_a, secrecy_b);
// Result: (party_a, "mutual_forbearance_confidentiality_a_confidentiality_b", party_b)
```

---

## Non-Movable Object Contextual Joins

### `encumber(asset1, asset2)`

**Domain**: Non-movable object ↔ Positive service  
**Pattern**: Reciprocal - `(s1, A1, s2)` and `(s2, A2, s1)`  
**Type Checking**: A1 = non-movable object action, A2 = positive service action  
**Semantics**: Security interest or encumbrance on property

#### Rules
```witness
encumber((s1, A1, s2), (s2, A2, s1)):
    if A1.object.constraint == non_movable AND A2.service.constraint == positive:
        return (s1, "encumbered_A1.object_with_A2.service", s2)
    else:
        ERROR("encumber requires non-movable object and positive service")
```

#### Example
```witness
object land = "real estate property", non_movable;
service mortgage_guarantee = "loan security", positive;
action mortgage_land = "mortgage", land;
action provide_guarantee = "provide", mortgage_guarantee;

asset land_mortgage = owner, mortgage_land, bank;           // (owner, mortgage_land, bank)
asset guarantee_service = bank, provide_guarantee, owner;   // (bank, provide_guarantee, owner)

asset secured_property = encumber(land_mortgage, guarantee_service);
// Result: (owner, "encumbered_land_with_mortgage_guarantee", bank)
```

### `access(asset1, asset2)`

**Domain**: Non-movable object ↔ Positive service  
**Pattern**: Reciprocal - `(s1, A1, s2)` and `(s2, A2, s1)`  
**Type Checking**: A1 = non-movable object action, A2 = positive service action  
**Semantics**: Usage rights over non-movable property

#### Rules
```witness
access((s1, A1, s2), (s2, A2, s1)):
    if A1.object.constraint == non_movable AND A2.service.constraint == positive:
        return (s1, "granted_access_A1.object_for_A2.service", s2)
    else:
        ERROR("access requires non-movable object and positive service")
```

#### Example
```witness
object office_building = "commercial property", non_movable;
service rent_payment = "rental fee", positive;
action lease_office = "lease", office_building;
action pay_rent = "pay", rent_payment;

asset lease_grant = landlord, lease_office, tenant;     // (landlord, lease_office, tenant)
asset rent_obligation = tenant, pay_rent, landlord;     // (tenant, pay_rent, landlord)

asset tenancy = access(lease_grant, rent_obligation);
// Result: (landlord, "granted_access_office_building_for_rent_payment", tenant)
```

### `lien(asset1, asset2)`

**Domain**: Non-movable object ↔ Negative service  
**Pattern**: Reciprocal - `(s1, A1, s2)` and `(s2, A2, s1)`  
**Type Checking**: A1 = non-movable object action, A2 = negative service action  
**Semantics**: Restricted ownership with negative obligations

#### Rules
```witness
lien((s1, A1, s2), (s2, A2, s1)):
    if A1.object.constraint == non_movable AND A2.service.constraint == negative:
        return (s1, "lien_A1.object_restricted_by_A2.service", s2)
    else:
        ERROR("lien requires non-movable object and negative service")
```

#### Example
```witness
object commercial_property = "business premises", non_movable;
service sale_restriction = "sales prohibition", negative;
action encumber_property = "encumber", commercial_property;
action abstain_sale = "abstain from", sale_restriction;

asset property_lien = debtor, encumber_property, creditor;    // (debtor, encumber_property, creditor)
asset sale_prohibition = debtor, abstain_sale, creditor;      // (debtor, abstain_sale, creditor)

asset restricted_ownership = lien(property_lien, sale_prohibition);
// Result: (debtor, "lien_commercial_property_restricted_by_sale_restriction", creditor)
```

---

## Universal Evidence and Argument Joins

### `evidence(asset1, asset2)`

**Domain**: Any asset ↔ Any asset (universal)  
**Pattern**: No reciprocal requirement  
**Type Checking**: None  
**Semantics**: Links supporting evidence to legal claims

#### Rules
```witness
evidence(legal_claim, supporting_evidence):
    return (legal_claim.s1, "legal_claim.action_supported_by_supporting_evidence.action", legal_claim.s2)
```

#### Example
```witness
asset breach_claim = plaintiff, allege_breach, defendant;
asset witness_testimony = witness, testify, court;
asset contract_document = notary, present_contract, court;

asset evidenced_claim = evidence(breach_claim, witness_testimony);
// Result: (plaintiff, "allege_breach_supported_by_testify", defendant)

asset strong_evidence = evidence(evidenced_claim, contract_document);
// Result: (plaintiff, "allege_breach_supported_by_testify_supported_by_present_contract", defendant)
```

#### Use Cases
- Building evidence chains
- Linking claims to supporting materials
- Creating documented legal positions

### `argument(asset1, asset2)`

**Domain**: Any asset ↔ Any asset (universal)  
**Pattern**: No reciprocal requirement  
**Type Checking**: None  
**Semantics**: Constructs legal arguments from components

#### Rules
```witness
argument(evidenced_claim, supporting_claim):
    return (evidenced_claim.s1, "argued_evidenced_claim.action_with_supporting_claim.action", evidenced_claim.s2)
```

#### Example
```witness
asset evidenced_breach = evidence(breach_claim, witness_testimony);
asset statutory_authority = lawyer, cite_statute, court;
asset precedent_case = lawyer, cite_precedent, court;

asset legal_argument = argument(evidenced_breach, statutory_authority);
// Result: (plaintiff, "argued_allege_breach_supported_by_testify_with_cite_statute", defendant)

asset complete_argument = argument(legal_argument, precedent_case);
// Result: (plaintiff, "argued_argued_allege_breach_supported_by_testify_with_cite_statute_with_cite_precedent", defendant)
```

#### Use Cases
- Constructing complex legal arguments
- Combining multiple legal authorities
- Building adversarial positions

---

## System Operations

System operations provide global analysis and domain-specific aggregation capabilities.

### `global()`

**Domain**: Global program state  
**Arguments**: None  
**Type Checking**: None  
**Semantics**: Performs satisfiability checking and generates validity witness

#### Rules
```witness
global():
    // Analyzes all clauses and assets in current scope
    // Performs satisfiability checking
    // Returns validity witness asset if satisfiable
    // Triggers truth table generation
```

#### Example
```witness
clause offer = oblig(seller_offer);
clause acceptance = oblig(buyer_acceptance);
clause consideration = oblig(payment) AND oblig(delivery);

asset contract_valid = global();
// Checks satisfiability of all clauses
// Generates truth table for analysis
```

#### Use Cases
- Contract validity verification
- Legal consistency checking
- Satisfiability analysis
- Truth table generation

### `domain(asset1, asset2, ...)`

**Domain**: Related legal concepts  
**Arguments**: Multiple assets or clauses  
**Type Checking**: Compatible asset types  
**Semantics**: Aggregates related legal concepts for domain analysis

#### Rules
```witness
domain(assets...):
    // Analyzes relationships between domain-specific assets
    // Groups related legal concepts
    // Returns aggregated domain analysis
```

#### Example
```witness
asset capacity_analysis = domain(
    procedural_capacity_valid,
    substantive_capacity_valid,
    temporal_capacity_valid
);
```

#### Use Cases
- Multi-factor legal analysis
- Capacity determination
- Related obligation grouping

### `litis(asset1, asset2)`

**Domain**: Litigation and dispute resolution  
**Arguments**: Two opposing legal positions  
**Type Checking**: Compatible legal claims  
**Semantics**: Models adversarial legal proceedings

#### Rules
```witness
litis(claim_a, claim_b):
    // Models litigation between opposing claims
    // Analyzes adversarial positions
    // Returns litigation analysis asset
```

#### Example
```witness
asset breach_claim = plaintiff, allege_breach, defendant;
asset defense_claim = defendant, deny_breach, plaintiff;

asset litigation = litis(breach_claim, defense_claim);
```

#### Use Cases
- Dispute modeling
- Adversarial analysis
- Litigation strategy

### `meet(asset1, asset2)`

**Domain**: Asset intersection  
**Arguments**: Two assets with common elements  
**Type Checking**: Compatible asset structures  
**Semantics**: Extracts common elements from legal structures

#### Rules
```witness
meet(asset_a, asset_b):
    // Finds intersection of legal obligations
    // Extracts common elements
    // Returns minimal common structure
```

#### Example
```witness
asset common_obligations = meet(
    contract_obligations,
    statutory_obligations
);
```

#### Use Cases
- Common obligation extraction
- Legal requirement intersection
- Minimal compliance determination

---

## Logical Operations

Logical operations provide building blocks for constructing legal clauses and assertions.

### `oblig(asset)`

**Domain**: Legal obligations  
**Arguments**: Single asset representing an obligation  
**Type Checking**: Valid asset structure  
**Semantics**: Marks an asset as a legal obligation

#### Rules
```witness
oblig(asset):
    // Creates obligation assertion
    // Marks asset as required/mandatory
    // Used in clause construction
```

#### Example
```witness
asset payment_obligation = buyer, pay_money, seller;
clause contract_term = oblig(payment_obligation);

// In logical expressions
clause contract = oblig(offer) AND oblig(acceptance) AND oblig(consideration);
```

#### Use Cases
- Obligation declaration
- Contract term specification
- Legal requirement assertion

### `claim(asset)`

**Domain**: Legal claims  
**Arguments**: Single asset representing a claim  
**Type Checking**: Valid asset structure  
**Semantics**: Marks an asset as a legal claim or assertion

#### Rules
```witness
claim(asset):
    // Creates claim assertion
    // Marks asset as asserted/claimed
    // Used in dispute and argumentation
```

#### Example
```witness
asset breach_allegation = plaintiff, allege_breach, defendant;
clause breach_claim = claim(breach_allegation);

clause dispute = claim(breach_allegation) XOR claim(defense_position);
```

#### Use Cases
- Legal claim assertion
- Dispute position declaration
- Argumentation structure

### `not(asset)`

**Domain**: Logical negation  
**Arguments**: Single asset or logical expression  
**Type Checking**: Valid logical expression  
**Semantics**: Logical negation of the given asset or expression

#### Rules
```witness
not(asset):
    // Logical negation
    // Creates complement of logical expression
    // Used in conditional and breach analysis
```

#### Example
```witness
asset payment_made = buyer, pay_money, seller;
clause breach_condition = not(payment_made) OR not(delivery_made);

clause contract_valid = oblig(payment_made) AND not(breach_occurred);
```

#### Use Cases
- Breach condition modeling
- Conditional logic
- Complementary assertions
- Violation detection

---

## Error Handling

### Common Error Conditions

1. **Type Constraint Violations**
   ```witness
   // ERROR: transfer requires movable objects
   object house = "property", non_movable;
   asset invalid_transfer = transfer(house_sale, house_delivery);
   ```

2. **Pattern Mismatches**
   ```witness
   // ERROR: Not reciprocal pattern
   asset invalid_sell = sell((seller, sell_house, buyer), (seller, pay_price, buyer));
   ```

3. **Service Constraint Violations**
   ```witness
   // ERROR: compensation requires positive services
   service negative_service = "restriction", negative;
   asset invalid_compensation = compensation(positive_service_asset, negative_service_asset);
   ```

### Error Messages

The Witness compiler provides specific error messages for join operation failures:

```
Error: transfer operation failed
  → Line 45: transfer(house_sale, cash_payment)
  → Reason: transfer requires movable object actions
  → Found: house (non_movable), cash (movable)
  → Expected: both objects must be movable

Error: sell operation failed  
  → Line 67: sell(service_asset, service_asset)
  → Reason: sell requires object action and positive service action
  → Found: positive service, positive service
  → Expected: object action ↔ positive service action

Error: reciprocal pattern mismatch
  → Line 23: compensation(asset1, asset2)
  → Reason: compensation requires reciprocal pattern (s1,A1,s2) ↔ (s2,A2,s1)
  → Found: (lawyer, provide_service, client) ↔ (accountant, provide_service, client)
  → Expected: (lawyer, provide_service, client) ↔ (client, provide_service, lawyer)
```

---

## Summary Table

| Operation | Category | Domain | Pattern | Semantics |
|-----------|----------|---------|---------|-----------|
| `join` | Universal Join | Universal | None | Forceful combination |
| `transfer` | Contextual Join | movable ↔ movable | Reciprocal | Physical transfer |
| `sell` | Contextual Join | object ↔ positive service | Reciprocal | Commercial exchange |
| `compensation` | Contextual Join | positive ↔ positive | Reciprocal | Service exchange |
| `consideration` | Contextual Join | positive ↔ negative | Reciprocal | Mixed obligation |
| `forbearance` | Contextual Join | negative ↔ negative | Reciprocal | Mutual abstention |
| `encumber` | Contextual Join | non-movable ↔ positive | Reciprocal | Security interest |
| `access` | Contextual Join | non-movable ↔ positive | Reciprocal | Usage rights |
| `lien` | Contextual Join | non-movable ↔ negative | Reciprocal | Restricted ownership |
| `evidence` | Universal Join | Universal | None | Evidential support |
| `argument` | Universal Join | Universal | None | Legal argumentation |
| `global()` | System Operation | Global state | None | Satisfiability checking |
| `domain()` | System Operation | Related concepts | Multiple assets | Domain aggregation |
| `litis()` | System Operation | Adversarial | Two claims | Litigation modeling |
| `meet()` | System Operation | Asset intersection | Two assets | Common elements |
| `oblig()` | Logical Operation | Obligations | Single asset | Obligation assertion |
| `claim()` | Logical Operation | Claims | Single asset | Claim assertion |
| `not()` | Logical Operation | Negation | Single expression | Logical negation |

---

*For more information, see the [Witness Language Documentation](README.md)*