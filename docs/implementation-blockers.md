# v0.1 implementation blockers

these are places where the current language contract does not provide enough
information to implement one interoperable answer. this file records the gap.
it does not propose the answer.

## status meanings

| status     | meaning                                                                          |
| ---------- | -------------------------------------------------------------------------------- |
| `blocking` | the named roadmap work cannot honestly ship until normative text resolves it     |
| `partial`  | specified behavior may be implemented, but the unresolved edge stays unavailable |
| `excluded` | the feature is not in the current v0.1 implementation scope                      |

## blocker ledger

| id                 | status   | missing normative contract                                                                                                                                                | affected roadmap work                                   |
| ------------------ | -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------- |
| `BLK-LEX-001`      | blocking | source identifier start/continue characters and normalization are not defined; the package-name regex is not a source identifier grammar                                  | `002`, `013`, `016`, `017–024`, `126`                   |
| `BLK-LEX-002`      | blocking | the closed reserved-keyword set is not stated; a grammar literal does not by itself prove reservation                                                                     | `002`, `013`, `016`, `126`                              |
| `BLK-LEX-003`      | blocking | accepted whitespace/line endings plus line, block and documentation comment delimiters, nesting and termination are not defined                                           | `002`, `013`, `016`, `023`, `126`, `128`                |
| `BLK-LEX-004`      | blocking | integer/float base forms, separators, exponent forms and suffix grammar are not closed                                                                                    | `002`, `014`, `016`, `034–035`, `126`                   |
| `BLK-LEX-005`      | blocking | string/character escape spellings and invalid escape handling are not defined                                                                                             | `002`, `015–016`, `126`                                 |
| `BLK-LEX-006`      | blocking | the complete operator spellings and lexical longest-match rules are not defined                                                                                           | `002`, `013`, `016`, `019`, `126`                       |
| `BLK-SYN-001`      | blocking | there is no complete declaration/expression grammar, normative modifier order or closed precedence/associativity table                                                    | `003`, `017`, `019–023`, `126`, `128`                   |
| `BLK-SYN-002`      | blocking | examples establish the basic constructor-definition shape, but default construction, constructor modifiers, destructors and clone-constructor declarations are incomplete | `003`, `021`, `039`, `053–061`, `113–120`               |
| `BLK-SYN-003`      | blocking | trait declaration bodies, required/default method syntax and full `impl` body grammar are not shown                                                                       | `003`, `022`, `031`, `040`, `065–069`, `106`, `112`     |
| `BLK-SYN-004`      | blocking | payload-enum declaration and full pattern grammar are incomplete; matching behavior is defined more completely than its declaration syntax                                | `003`, `022`, `038`, `073–080`, `126`                   |
| `BLK-SYN-005`      | partial  | common attributes are described individually, but attachment sites, ordering, repetition and a closed argument grammar are not defined                                    | `003`, `023`, `026`, `093`, `097–099`, `125`, `127–128` |
| `BLK-SYN-006`      | partial  | ordinary global mutable declarations and their storage/initialization rules are not defined; foreign globals are explicitly deferred                                      | `003`, `021`, `042`, `046`, `048`                       |
| `BLK-COMPTIME-001` | blocking | general `comptime` declaration/call syntax and the mechanism that marks a function as permitted are not defined                                                           | `003`, `023`, `040`, `087–088`, `103`, `128`            |
| `BLK-COMPTIME-002` | partial  | resource exhaustion, recursion/step limits and the stability of those limits are not specified                                                                            | `087–088`, `121`, `133`                                 |
| `BLK-ENTRY-001`    | blocking | valid executable entry signatures, argument delivery and return mapping are not defined                                                                                   | `007`, `048`, `103`, `120`, `130`, `134–136`            |
| `BLK-ENTRY-002`    | blocking | runtime initialization order and process-wide/static destruction are not defined                                                                                          | `007`, `048`, `054`, `120`, `134`                       |
| `BLK-REGISTRY-001` | blocking | registry endpoint discovery, configuration, transport API and authentication are not defined                                                                              | `007`, `100–101`, `104`, `134–136`                      |
| `BLK-REGISTRY-002` | blocking | source archive format, canonical file ordering, checksum algorithm and publish/fetch wire behavior are not defined                                                        | `007`, `100–101`, `104`, `134–136`                      |
| `BLK-LIB-001`      | partial  | section 10 defines library behavior and many signatures, but not a closed declaration-level API for every listed type/error/operation                                     | `105–120`, `128`, `135`                                 |
| `BLK-DIAG-001`     | partial  | diagnostic codes must be stable, but the initial code registry is not specified                                                                                           | `012`, `016`, `024`, `032`, `121–122`, `127`, `133`     |
| `BLK-LANG-001`     | excluded | properties are mentioned by visibility prose but have no normative syntax or semantics                                                                                    | none; do not parse, type or lower properties in v0.1    |
| `BLK-LANG-002`     | excluded | a GC wrapper is named only by an ABI rejection; allocation, roots, tracing and finalization do not exist normatively                                                      | none; class ownership remains RAII and allocator-backed |
| `BLK-LANG-003`     | excluded | async/await has no normative source syntax, future representation, suspension or cancellation contract                                                                    | none; `std` remains synchronous                         |

## implementation rule

an implementation commit may proceed around a `partial` blocker only for the
behavior already stated by the language rules. it may not fill the missing edge with an
undocumented default and call that compatibility.

a `blocking` item must be resolved in a future normative revision before the
affected behavior ships. changing this ledger to “resolved” without a matching
a matching language-rule update is required. changing only the ledger is moving
the problem into a table.

the three `excluded` entries stay rejected or unrecognized in v0.1. they are
not compiler extension points, hidden flags or syntax reserved for later.
